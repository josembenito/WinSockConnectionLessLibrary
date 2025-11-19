# WinSock UDP Connectionless Library

Warning: this is an quick prototype and should be used as an example only for research purposes.

This is a simple Socket Communications Library for connecionless UDP datagrams (just open your socket and you are good to go) communications. Great for LAN environments where simplicity and speed are paramount.
You need a reliable connection (UDP gives no garantees as opposed to TCP) and you need to know the IPs of each machine to connect to in the network. 
Sockets used with connectionless datagram services need not be connected before they are used, but connecting sockets makes it possible to transfer data without specifying the destination each time.
For more information and to grasp the idea please check: https://www.ibm.com/docs/en/i/7.4.0?topic=socket-example-connectionless-client

This project is divided in Modules

SocketCommunications: is where the core functionality is
The rest are examples that work in pairs: 
AsycUdpClient with AsyncUdpServer
&
∑ocketSender with SocketListener

``

Copyright 2025 Jose Maria Benito Diaz

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


