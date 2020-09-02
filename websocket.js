/**
 * websocket 编程
 * @author Philip
 */

const ws = require("nodejs-websocket");

const server = ws.createServer(function (conn) {
    conn.on("text", function (str) {
        conn.sendText(str);
    });

    conn.on("close", function (code, reason) {
        console.log("关闭连接");
    });

    conn.on("error", function (code, reason) {
        console.log("异常关闭");
    });
}).listen(3333);
