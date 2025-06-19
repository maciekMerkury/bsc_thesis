// static_http_server.js
const http = require('http');

const html = `
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Simple Node Server</title>
  <style>
    body { font-family: Arial, sans-serif; padding: 2rem; background: #f9f9f9; }
    h1 { color: #333; }
  </style>
</head>
<body>
  <h1>Hello from Node.js!</h1>
  <p>This HTML is served from a simple static HTTP server.</p>
</body>
</html>
`;

const server = http.createServer((req, res) => {
  if (req.url === '/') {
    res.writeHead(200, {
      'Content-Type': 'text/html',
      'Content-Length': Buffer.byteLength(html)
    });
    res.end(html);
  } else {
    res.writeHead(404, { 'Content-Type': 'text/plain' });
    res.end('404 Not Found');
  }
});

const PORT = 8080;
server.listen(PORT, '127.0.0.1', () => {
  console.log(`Server running at http://localhost:${PORT}/`);
});

