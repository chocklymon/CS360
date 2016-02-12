Lab 4
================

When you wrote the code for a web client, you waited for a response before issuing another request.  In this
lab you will write a client that will issue a large number of requests and then asynchronously receive the
responses.  This will allow you to perform tests to see how good your web server really is.

Your lab will implement the following functionality:

 - You should have a makefile that creates a binary called `webtest` with the parameters `webtest <url> -d <count>`.
 - When you run webtest it will create <count> sockets, and will issue a request for the <url> on each socket.
 - The webtest program will then use epoll to retrieve responses once they come in.
 - Time the latency of the responses and calculate the average response time and the standard deviation and print
   them to the screen.
 - If the -d flag is used, print out the response time for each request.

_Note:_ When submitting specify how it should be run.

Passoff
--------------

Please submit your code through Learning Suite. Compress your code in the same manner in which you compressed
your web client.

| Passoff Level   | Behavior                                                       | Points |
+-----------------+----------------------------------------------------------------+--------+
| Minimal Passoff | You issue <count> HTTP requests to the web server correctly.   |    25% |
| Basic Passoff   | You use epoll to wait for the responses with a single thread.  |    50% |
| Better          | You report on the response time with the -d option             |    75% |
| Perfect         | You correctly calculate the average response time and standard |   100% |
                    deviation and print them out
