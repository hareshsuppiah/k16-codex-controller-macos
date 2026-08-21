import http from "node:http";
import https from "node:https";

const upstream = "www.sdcx-tech.com";
const port = 8765;
const sourceLayout = "./0816_2475.json";
const targetLayout = "./36ae_2475.json";

const server = http.createServer((request, response) => {
  if (request.method !== "GET" && request.method !== "HEAD") {
    response.writeHead(405, { "content-type": "text/plain; charset=utf-8" });
    response.end("Only read-only configurator requests are allowed.");
    return;
  }

  const options = {
    hostname: upstream,
    port: 443,
    // The site bundle is patched below so the real K16 VID/PID resolves to
    // the closest supported 16-key/3-knob layout.  The upstream server does
    // not actually contain a 36ae_2475.json file, so serve the related layout
    // whenever the patched app requests that path.
    path: request.url.replace("/36ae_2475.json", "/0816_2475.json"),
    method: request.method,
    headers: {
      host: upstream,
      accept: request.headers.accept ?? "*/*",
      "accept-language": request.headers["accept-language"] ?? "en-AU,en;q=0.9",
      "user-agent": request.headers["user-agent"] ?? "Mozilla/5.0",
      "accept-encoding": "identity",
    },
  };

  const upstreamRequest = https.request(options, (upstreamResponse) => {
    const chunks = [];
    upstreamResponse.on("data", (chunk) => chunks.push(chunk));
    upstreamResponse.on("end", () => {
      let body = Buffer.concat(chunks);
      const contentType = upstreamResponse.headers["content-type"] ?? "";

      if (contentType.includes("javascript")) {
        const script = body.toString("utf8");
        if (script.includes(sourceLayout)) {
          body = Buffer.from(script.replace(sourceLayout, targetLayout), "utf8");
        }
      }

      const headers = { ...upstreamResponse.headers };
      delete headers["content-encoding"];
      delete headers["transfer-encoding"];
      delete headers["content-security-policy"];
      delete headers["content-security-policy-report-only"];
      headers["content-length"] = String(body.length);
      headers["cache-control"] = "no-store";

      response.writeHead(upstreamResponse.statusCode ?? 502, headers);
      response.end(body);
    });
  });

  upstreamRequest.on("error", (error) => {
    response.writeHead(502, { "content-type": "text/plain; charset=utf-8" });
    response.end(`Unable to reach configurator: ${error.message}`);
  });

  upstreamRequest.end();
});

server.listen(port, "127.0.0.1", () => {
  console.log(`K16 layout workaround ready at http://localhost:${port}`);
  console.log(`Mapping ${targetLayout} to the site's ${sourceLayout} layout.`);
});
