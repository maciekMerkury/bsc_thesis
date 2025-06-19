// echo_server.js
const net = require('net');

const server = net.createServer((socket) => {
  console.log('Client connected');

  socket.on('data', (data) => {
    console.log(`Received: ${data}`);
    socket.write(data); // Echo back
  });

  socket.on('end', () => {
    console.log('Client disconnected');
  });
});

server.listen(4000, '127.0.0.1', () => {
  console.log('Echo server listening on port 4000');
});

