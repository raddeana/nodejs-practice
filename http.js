/**
 * Http
 * @author Chenxiangyu
 */
const http = require('http');

http.createServer(function (request, response) {
    response.end("Hello World\n");
}).listen(9297);