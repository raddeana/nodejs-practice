/**
 * tcp
 * @author unknow
 */

const { TCP } = process.binding('tcp_wrap');
const tcp = new TCP();
tcp.listen(3000);