import { spawn, spawnSync } from "node:child_process";
import { createServer } from "node:http";
import { existsSync, mkdirSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = dirname(fileURLToPath(import.meta.url));
const binaryDirectory = join(projectRoot, ".bin");
const port = Number(process.env.CUBELAB_PORT || 5173);
const movePattern = /^[URFDLB](?:2|')?$/;

const staticFiles = new Map([
  ["/", ["index.html", "text/html; charset=utf-8"]],
  ["/index.html", ["index.html", "text/html; charset=utf-8"]],
  ["/styles.css", ["styles.css", "text/css; charset=utf-8"]],
  ["/app.js", ["app.js", "text/javascript; charset=utf-8"]],
]);

function compileNativeSolvers() {
  mkdirSync(binaryDirectory, { recursive: true });
  const builds = [
    {
      name: "Thistlethwaite",
      output: join(binaryDirectory, "thistlethwaite_solver"),
      args: [
        "-std=c++17",
        "-O3",
        `-I${join(projectRoot, "Thistlethwaite_algorithm")}`,
        join(projectRoot, "Thistlethwaite_algorithm/solver_cli.cpp"),
        join(projectRoot, "Thistlethwaite_algorithm/Model/CubieCube.cpp"),
        join(projectRoot, "Thistlethwaite_algorithm/PatternDatabases/PhaseCoordinates.cpp"),
        join(projectRoot, "Thistlethwaite_algorithm/PatternDatabases/PhasePatternDatabases.cpp"),
        join(projectRoot, "Thistlethwaite_algorithm/Solver/ThistlethwaiteSolver.cpp"),
      ],
    },
    {
      name: "Korf IDA*",
      output: join(binaryDirectory, "korf_solver"),
      args: [
        "-std=c++17",
        "-O3",
        `-I${join(projectRoot, "Korf_algorithm")}`,
        join(projectRoot, "Korf_algorithm/solver_cli.cpp"),
        join(projectRoot, "Korf_algorithm/Model/CubieCube.cpp"),
        join(projectRoot, "Korf_algorithm/PatternDatabases/MappedNibbleDatabase.cpp"),
        join(projectRoot, "Korf_algorithm/PatternDatabases/KorfHeuristic.cpp"),
        join(projectRoot, "Korf_algorithm/Solver/NonRecursiveIDAStar.cpp"),
      ],
    },
  ];

  for (const build of builds) {
    const result = spawnSync("c++", [...build.args, "-o", build.output], {
      cwd: projectRoot,
      stdio: "inherit",
    });
    if (result.error) throw result.error;
    if (result.status !== 0) throw new Error(`${build.name} compilation failed.`);
  }
}

function sendJson(response, status, payload) {
  response.writeHead(status, {
    "Content-Type": "application/json; charset=utf-8",
    "Cache-Control": "no-store",
  });
  response.end(JSON.stringify(payload));
}

function normalizeScramble(value) {
  if (typeof value !== "string") throw new Error("Scramble must be text.");
  const moves = value.trim() ? value.trim().split(/\s+/) : [];
  if (moves.length > 100) throw new Error("Scramble exceeds 100 turns.");
  const invalid = moves.find((move) => !movePattern.test(move));
  if (invalid) throw new Error(`Unsupported move: ${invalid}`);
  return moves.join(" ");
}

function runNativeSolver(algorithm, scramble) {
  return new Promise((resolve, reject) => {
    const isThistlethwaite = algorithm === "thistlethwaite";
    const executable = join(
      binaryDirectory,
      isThistlethwaite ? "thistlethwaite_solver" : "korf_solver",
    );
    const workingDirectory = isThistlethwaite
      ? projectRoot
      : join(projectRoot, "Korf_algorithm");
    const args = isThistlethwaite
      ? [scramble, join(projectRoot, "Thistlethwaite_algorithm/Databases")]
      : [scramble, "Databases"];
    const timeoutMs = isThistlethwaite ? 120_000 : 60_000;
    const child = spawn(executable, args, {
      cwd: workingDirectory,
      stdio: ["ignore", "pipe", "pipe"],
    });

    let output = "";
    let errorOutput = "";
    let timedOut = false;
    const timeout = setTimeout(() => {
      timedOut = true;
      child.kill("SIGKILL");
    }, timeoutMs);

    child.stdout.on("data", (chunk) => {
      output += chunk;
      if (output.length > 1_000_000) child.kill("SIGKILL");
    });
    child.stderr.on("data", (chunk) => {
      errorOutput += chunk;
    });
    child.on("error", reject);
    child.on("close", (code) => {
      clearTimeout(timeout);
      if (timedOut) {
        reject(
          new Error(
            `${isThistlethwaite ? "Thistlethwaite" : "Korf IDA*"} exceeded its local time limit. Try a shorter scramble.`,
          ),
        );
        return;
      }
      if (code !== 0) {
        reject(new Error(errorOutput.trim() || `Native solver exited with code ${code}.`));
        return;
      }
      try {
        resolve(JSON.parse(output));
      } catch {
        reject(new Error("Native solver returned malformed output."));
      }
    });
  });
}

function serveStaticFile(pathname, response) {
  const entry = staticFiles.get(pathname);
  if (!entry) return false;
  const [fileName, contentType] = entry;
  const filePath = join(projectRoot, "frontend", fileName);
  if (!existsSync(filePath)) {
    sendJson(response, 404, { error: "Frontend file not found." });
    return true;
  }
  response.writeHead(200, {
    "Content-Type": contentType,
    "Cache-Control": "no-cache",
  });
  response.end(readFileSync(filePath));
  return true;
}

function readRequestBody(request) {
  return new Promise((resolve, reject) => {
    let body = "";
    request.on("data", (chunk) => {
      body += chunk;
      if (body.length > 20_000) reject(new Error("Request is too large."));
    });
    request.on("end", () => resolve(body));
    request.on("error", reject);
  });
}

compileNativeSolvers();

const server = createServer(async (request, response) => {
  const pathname = new URL(request.url || "/", "http://localhost").pathname;
  if (request.method === "GET" && serveStaticFile(pathname, response)) return;

  if (request.method === "GET" && pathname === "/api/health") {
    sendJson(response, 200, { ready: true, algorithms: ["thistlethwaite", "korf"] });
    return;
  }

  if (request.method !== "POST" || pathname !== "/api/solve") {
    sendJson(response, 404, { error: "Not found" });
    return;
  }

  try {
    const body = await readRequestBody(request);
    const payload = JSON.parse(body || "{}");
    if (!new Set(["thistlethwaite", "korf"]).has(payload.algorithm)) {
      throw new Error("Choose Thistlethwaite or Korf IDA*.");
    }
    const scramble = normalizeScramble(payload.scramble);
    const result = await runNativeSolver(payload.algorithm, scramble);
    sendJson(response, 200, result);
  } catch (error) {
    sendJson(response, 400, {
      error: error instanceof Error ? error.message : "Solver request failed.",
    });
  }
});

server.listen(port, "127.0.0.1", () => {
  console.log(`CubeLab: http://localhost:${port}`);
  console.log("Press Ctrl+C to stop.");
});
