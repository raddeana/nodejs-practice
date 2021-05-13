/**
 * dns
 * @author unknow
 */

const dns = require('dns');

dns.lookup('www.baidu.com', function (err, address, family) {
	console.log(err);
  console.log(address);
  console.log(family);
});