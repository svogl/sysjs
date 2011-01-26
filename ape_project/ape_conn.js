load("WS_Socket.js");
load("APE_Proto.js");

var sock = new WS_Socket("localhost.localdomain", 6969);
var proto = new APE_Proto(sock);
var ctr=0;

try {
var cc = proto.newCmd( "CONNECT", {"name": "f"+new Date().getTime() })
var cj = proto.newCmd("JOIN", {"channels": "test" });

proto.start();

print(":");

sock.send( [ cc , cj ] );

print(":" );


stdin = new Socket(0);
stderr = new Socket(2);

stderr.log = function(msg) {
	this.write("[ERR] "+msg+"\n");
}


var channel = proto.state.names['test'];

while (true) {
	print(".");
	stdin.poll( [ sock.s ], 300);

	ctr++;
	if ( ctr % 6 == 0 ) {

		channel.send("Testing... "+ctr);
		gc();
	}

}

} catch(e) {
	stderr.log(e);
	stderr.log(e.stack);
}

