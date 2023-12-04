# Thread-safe Caching Web Proxy

## Team members
Jiuao Song, Zixuan Wang

## Related course material
- Multi-threading: in Part II, we use multi-threading to deal with concurrent requests.
- Socket Programming

## System Requirements and Dependencies
### Operating System
- Linux
### Required Tools
- curl(Version 7.2 or higher)
- telnet(optional)

## Compilation

### Files
You can see there are 3 proxy-related cpp files in our folder. 

`proxy.cpp` implements a basic sequential proxy that handles HTTP GET requests. 

`proxy2.cpp` implements a concurrent server based on `proxy.cpp`, which spawns a new thread to handle each new connection. Our threads run in detached mode to avoid memory leaks. 

`proxy3.cpp` implements a cache to our proxy based on `proxy2.cpp` which stores web objects in memory. When the proxy receives an object from the server, it will cache it in the memory. If another client requests the same object from the same server, the proxy can simply resend the cache. Our cache also has a LRU eviction policy.

`ProxyConcurrentTest.sh` is our auto-testing script for testing concurrency.

`ProxyCacheTest.sh` is our auto-testing script for testing the cache function.

### Compile with `proxy.cpp`
```
g++ proxy.cpp csapp.cpp -o test -lpthread
```

This will create `test` in current working directory. 

Now, open a new terminal. With the following command, our proxy listens for connections on port 30308
```
linux> ./test 30308
```
Then, on the client side, you have two primary options
#### 1) telnet
Use Telnet to establish a connection with the proxy server. Execute the following command in your Linux terminal:
```
linux> telnet localhost 30308
```
This command connects you to the proxy server running on ``localhost`` at port ``30308``.  

Once you're connected to the proxy, you can test its functionality by making an HTTP request to a server such as Baidu. To do this, simply input the following HTTP GET request at the Telnet prompt:
```
GET http://baidu.com/ HTTP/1.1
```
The response from Baidu's server, including any headers, HTML content, and other data, will be sent back to your Telnet session, allowing you to view the raw HTTP response.
#### 2) `ProxyConcurrentTest.sh`
For automated testing without telnet, use the ``ProxyConcurrentTest.sh`` script. Ensure the ``CONCURRENCY`` variable is set to 1 for sequential requests.

To run the script, use:
```
linux> ./ProxyConcurrentTest.sh
```
### Compile with `proxy2.cpp`
```
g++ proxy2.cpp csapp.cpp -o test2 -lpthread
```

This will create `test2` in current working directory.

Now, open a new terminal and run
```
linux> ./test2 30308
```
Port 30308 is also hard-coded in the testing script.

Then, on the client side, run the ``ProxyConcurrentTest.sh`` script to test the concurrency.
```
linux> ./ProxyConcurrentTest.sh
```
This will show the response as well as successful response counts from the server. You can also customize the concurrency, port and host in the script.

### Compile with `proxy3.cpp`

```
g++ proxy3.cpp csapp.cpp -o test3 -lpthread
```

This will create `test3` in current working directory.

Now, open a new terminal and run
```
linux> ./test 30308
```

Next, you can run the ``ProxyCacheTest.sh`` to test the cache function.
```
linux> ./ProxyConcurrentTest.sh
```
This script runs multiple identical requests and measures their response times. A noticeable reduction in response time is expected, demonstrating the effectiveness of the cache.
