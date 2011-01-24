
stdin = new Socket(0);

s = new Socket();
s.data = function(d) {
	print(">>> "+d);
}

s.origin = "/";
s.uri = "/6/";
s.host = "0.localhost.localdom";

s.start_ws = function(url) {
    var headers = new Object();
    var connected = true;
    var line;
    print("REQ:" + "GET / HTTP/1.1\r\n");
    this.write("GET "+this.uri+" HTTP/1.1\r\n");
	this.write("Upgrade: WebSocket\r\n"); // N.B. header order is relevant
	this.write("Connection: Upgrade\r\n");
    this.write("Host: " + this.host + "\r\n");
	this.write("Origin: "+ this.origin +"\r\n");
	//WebSocket-Protocol optional
	this.write("Sec-WebSocket-Key1: 12 345 67 8\r\n");
	this.write("Sec-WebSocket-Protocol: sample\r\n");
	this.write("Sec-WebSocket-Key2: 1 2 34567 8\r\n");
    this.write("\r\n");
    this.write("12345678\r\n");
	this.write();
print("after head");
    // get answer:
    line = this.read(); // 101 OK, hopefully :)
print("after l");
    print("HEAD:: " + line);
    headers.response = line;
    while (connected) {
		line = this.read();
		if (line == null) {
			return null;
		}
		if (line.length == 0) {
			break;
		}
		var k = line.split(":",2);
		print("|| " + k[0] + "\t\t::: " + k[1] );
		headers[ k[0] ] = k[1];
    }
    chal = this.readBytes(16); // 16byte challenge follows, then we switch to ws mode...
	var i;
	var s="";
	for (i=0;i<chal.length;i++) {
		s += chal[i] + " ";
	}
	/** TODO: verify challenge
	*/
	print("done: " +s );
}

try {

conn = [{ "cmd":"CONNECT", "chl":0, "params":{"name":"testChannel","transport":0}}];
s.connect("0.localhost.localdom", 6969);

s.data = function() {
	print ("data?!? ");
	var l = this.read_ws_packet();
	print ("l "+l);
	var x = eval(l);
}

s.start_ws();

print(":");

s.write_ws_packet(JSON.stringify(conn));

print(":");

var l = s.read_ws_packet();

print(": "+l);

while (true) {
	print(".");
	s.poll( [ s ], 300);
}

} catch(e) {
	print(e);
	print(e.stack);
}

