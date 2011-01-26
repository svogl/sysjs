load("WS_Socket.js");


stdin = new Socket(0);


function APE_Pipe(proto, pipeData) {
	this.proto = proto;
	this.pipe = pipeData;

	this.handleData = function(msg) {
		var text = msg.msg;
		var from = msg.from;
		
		print( from.properties.name + " sez " + text);
	}

	this.send = function(msg) {
		var sendmsg = this.proto.newCmd( "SEND", {"msg": msg, "pipe" : this.pipe.pubid});
		print("send.. "+JSON.stringify(sendmsg));
		this.proto.sock.send( sendmsg );
	}

	this.handleJoin = function(msg) {
		print("joined.. ");
	}
	this.handleLeft = function(msg) {
		print("left.. ");
	}
}


function APE_Proto(socket) {
	this.chl= 0;
	this.sock = socket;
	this.state = { "connected": false };
	this.state.pipes = {};
	this.state.names = {};

	/**********************************/
	/* pipe funcs */
	this.handleChannel = function(chnl) {
		var p = chnl.pipe;
		var state = this.state;
		var pp = new APE_Pipe(this, p);

		state.pipes[p.pubid] = pp;
		state.names[p.properties.name] = pp;

		if (p.casttype == "multi" ) {
			print("multi found.");
		} else 
			return;

		var i, len;
		var users = chnl.users;
		len = users.length;
		for (i=0;i<len;i++) {
			var pp = new APE_Pipe(this, users[i]);
			state.pipes[users[i].pubid] = pp;
			state.names[users[i].properties.name] = pp;
		}
		print(JSON.stringify(this.pipes));
		print(JSON.stringify(this.names));
	}
	/** lookup the pipe, forward handling to pipe object: */
	this.handleData = function(msg) {
		var pid = msg.pipe.pubid;
		var state = this.state;
		if (state.pipes[pid]) {
			state.pipes[pid].handleData(msg);
		}
	}
	this.handleJoin = function(msg) {
		var pid = msg.pipe.pubid;
		var state = this.state;
		if (state.pipes[pid]) {
			state.pipes[pid].handleJoin(msg);
		}
	}
	this.handleLeft = function(msg) {
		var pid = msg.pipe.pubid;
		var state = this.state;
		if (state.pipes[pid]) {
			state.pipes[pid].handleLeft(msg);
		}
	}
	/**********************************/
	this.serverFuncs = {
		"err" : function(proto, msg) {
				print("Server error: " + msg.data.code + " : " + msg.data.value);
			},
		"login" : function(proto, msg) {
				print("login");
				proto.state.sessid = msg.data.sessid;
				proto.state.connected = true;
			},
		"ident" : function(proto, msg) {
				print("ident");
				proto.state.user = msg.data.user;
			},
		/*** pipe handling: */
		"channel" : function(proto, msg) {
				print("channel");
				proto.handleChannel(msg.data);
			},

		"data" : function(proto, msg) {
				print("data");
				proto.handleData(msg.data);
			},
		"join" : function(proto, msg) {
				print("data");
				proto.handleJoin(msg.data);
			},
		"left" : function(proto, msg) {
				print("data");
				proto.handleLeft(msg.data);
			},
	}

	this.handleMsg = function(raw) {
		var t = raw.raw.toLowerCase();
		try {
			this.serverFuncs[t](this, raw) ;
		} catch(e) {
			print("*** " + e);
			print("*** " + e.stack);
		}
	}

	this.handle = function(rawArr) {
		if (! rawArr) return;
		var l=rawArr.length;
		var i;
		for (i=0;i<l;i++) {
			print("handle" +  JSON.stringify(rawArr[i]));
			this.handleMsg( rawArr[i] );
		}
	}

	this.sock.handle = this.handle;

	this.newCmd = function(type, params,options) {
			var msg = {"cmd": type, "chl": this.chl++, "params": params };
			if (this.state.sessid) {
				msg.sessid = this.state.sessid;
			}
			if (options) {
				msg.options = options;
			}
			return msg;
		}

	this.start = function() {
		this.s.start_ws();
	}
};

var sock = new WS_Socket("localhost.localdomain", 6969);
var proto = new APE_Proto(sock);
var ctr=0;



try {
var cc = proto.newCmd( "CONNECT", {"name": "f"+new Date().getTime() })
var cj = proto.newCmd("JOIN", {"channels": "test" });

conn = [ cc , cj ];

sock.start_ws();

sock.handle = function(l) {
	print ("APE: "+l);
	try {
		proto.handle(l);
	} catch(e) {
		print(e);
		print(e.stack);
	}
}

print(":");

sock.send( conn );

print(":" );


while (true) {
	print(".");
	stdin.poll( [ sock.s ], 300);
	stdin.poll( [ stdin ], 250); // rate limit in case something breaks.

	ctr++;
	if (ctr%6 == 0) {

		proto.state.names['test'].send("FOOBAR "+ctr);
		gc();
	}

}

} catch(e) {
	print(e);
	print(e.stack);
}

