function WS_Socket(host, port) {
	this.s = new Socket();
	
	this.s.uri = "/6/"; // N.B.: 6 is a magic constant matching the enum in transport.h(!!!)
	this.s.host = host;
	this.s.port = port;
	this.s.origin = "http://"+this.host;

	/** start_ws tries to start a websocket connection to the APE server using 
	 * protocol version 0.76
	 */
	this.s.start_ws = function(req) {
		this.connect( this.host, this.port);


		var headers = new Object();
		var connected = true;
		var line;
		print("REQ:" + "GET / HTTP/1.1\r\n");
	//	var url = "/?" + JSON.stringify(req);
		this.write("GET " +this.uri+" HTTP/1.1\r\n");
		this.write("Upgrade: WebSocket\r\n"); // N.B. header order is relevant
		this.write("Connection: Upgrade\r\n");
		this.write("Host: " + this.host + "\r\n");
		this.write("Origin: "+ this.origin +"\r\n");
		//WebSocket-Protocol optional
		this.write("Sec-WebSocket-Key1: 12 345 67 8\r\n");
		this.write("Sec-WebSocket-Protocol: sample\r\n");
		this.write("Sec-WebSocket-Key2: 1 2 34567 8\r\n");
		this.write("Sec-WebSocket-Origin: "+this.origin+"\r\n");
		this.write("Sec-WebSocket-Location: "+this.uri+"\r\n");

		this.write("\r\n");
		this.write("12345678\r\n");

		// get answer:
		line = this.read(); // 101 OK, hopefully :)
		headers.response = line;

		while (connected) {
			line = this.read();
			if (line.length == 0) {
				break;
			}
			var k = line.split(":",2);
			print("|| " + k[0] + "\t\t::: " + k[1] );
			headers[ k[0] ] = k[1];
		}
		if (headers.response.indexOf("HTTP/1.1 101")==0) {
			this.isWS = true;
		} else {
			this.isWS = false;
			throw "TRAMPOLINE FAILED! - need 101 response!";
		}

		chal = this.readBytes(16); // 16byte challenge follows, then we switch to ws mode...
		var i;
		var str="";
		for (i=0;i<chal.length;i++) {
			str += chal[i] + " ";
		}
		/** TODO: verify challenge
		*/
		print("ws key3: " +str );
		this.headers = headers;
		this.hash = chal;
	}

	this.start_ws = function(req) {
		this.s.start_ws(req);
	}

	this.s.data = function() {
		try {
			var l = this.read_ws_packet();
			if (!l) 
				return;
			print ("[[APE]]: "+l);
			var x = eval(l);
			this.callback.handle(x);
		} catch(e) {
			print("*** " + e);
			print("*** " + e.stack);
		}
	}
	this.s.callback = this;

	this.handle = function(msg) {
		print("OVERRIDE ME. " + msg );
	}

	this.send = function(msg) {
		if (msg.length) {
			this.s.write_ws_packet(JSON.stringify( msg ));
		} else {
			this.s.write_ws_packet(JSON.stringify( [ msg ] ));
		}
	}
}

