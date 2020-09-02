/**
 * Http
 * @author Chenxiangyu
 */
var http = require('http');

http.createServer(function (request, response) {
    response.end('Hello World\n');
}).listen(9297);