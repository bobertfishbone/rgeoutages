var http = require('http');

http.createServer(function(request, respone){
  respone.writeHead(200, {'Content-type':'text/plain'});
  response.write('Hello Node JS Server Response');
  response.end( );

}).listen(7000);