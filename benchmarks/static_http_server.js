const http = require('http');
const fs = require('fs');

const PORT = 4000;
const IP = '127.0.0.1';

const html = fs.readFileSync("./static.html", "utf8");

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

server.listen(PORT, IP, () => {
  console.log(`Server running at http://${IP}:${PORT}/`);
});

