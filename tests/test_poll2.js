// Socket obj built in.


s1 = new Socket(0);
s2 = new Socket(1);
s3 = new Socket(2);

ss = new Socket();
ss.bind(8888);

var LOCAL = [s1,s2, s3];
var SOCKS = [ss];

SOCKS.remove = function(s) {
	var i;
	for (i=0;i<this.length;i++) {
		if (this[i]==s) {
			print("remove: found at " +i);
			this.splice(i,1);
			break;
		}
	}
}

function clientData() {
	var x= this.read();
	print("[recv: " + x + "]");
}
function clientError(errCode) {
	print("[err: " + errCode + "]");
	SOCKS.remove(this);
}

ss.data = function() {
	try {
		var sock = this.accept();
		sock.data = clientData;
		sock.error = clientError;
		print("got new " + sock);

		SOCKS[SOCKS.length] = sock;
	} catch(e) {
		print(e);
		print(e.stack);
	}
}

s1.data = function() {
	var x= this.read();
	s3.write("{" + x + "}");
}

s1.error = function() {
	running = false;
}

running = true;

do {
    Socket.poll(LOCAL, 300); // triggers callbacks above...
    Socket.poll(SOCKS, 300); // triggers callbacks above...
} while (running);
