// Socket obj built in.


s1 = new Socket(0);
s2 = new Socket(1);
s3 = new Socket(2);

var A = [s1,s2,s3];


s1.data = function() {
	var x= s1.read();
	s3.write("{" + x + "}");
}

s1.error = function() {
	running = false;
}

running = true;
print(A[0]);
print(A[1]);
print(A[2]);

do {
    Socket.poll(A, 1000); // triggers callbacks above...
} while (running);
