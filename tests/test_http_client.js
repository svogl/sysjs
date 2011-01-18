

var host="www.google.at";
var port=80;

stderr = new Socket(2);
stderr.log = function(l) {
	this.write("*** " + l + "\n");
}


function Get(host, port) {
	stderr.log("port " +this.port);
	this.host = host;
	this.port = port;
	this.open = false;
	this.inHeader = true;
	this.headers = {};
	this.body = "";
	stderr.log("port " +this.port);
	this.s = new Socket();
	this.s.error = function(code) {
		stderr.log("*** disconnect " + code);
		this.connected = false;
	}
	this.request = function(url) {
		try {
		this.connected = true;
		stderr.log("port " +this.port);
		this.s.connect(this.host, this.port);

		this.s.write("GET "+ url +" HTTP/1.0\r\n");
		this.s.write("Connection: close\r\n");
		this.s.write("\r\n");

		while (this.connected && this.inHeader) {
			var l = this.s.read(); // get a line;
			stderr.log(l);
			if (l.length==0) {
				this.inHeader = false;
			} else {
				var h = l.split(":",2);
				this.headers[h[0]] = h[1];
				stderr.log("H " + h[0] + " ::: " + h[1]);
			}
		}

		var len = parseInt(this.headers['Content-Length']);
		stderr.log("reading " + len);
		while (len>0 && this.connected) {
			l = this.s.read();
			this.body += l;
			len -= ( l.length +2 /* \r\n */ );
			stderr.log(len + " left ... " + l);
		}
		this.s.close();
		} catch(e) {
			stderr.log("EX " + e);
			stderr.log("EX " + e.stack);
		}
	}	
}

try {

stderr.log("port " +port);
var g = new Get(host, port);

g.request("/");


} catch (e) {
	stderr.log(e);
	stderr.log(e.stack);
}
