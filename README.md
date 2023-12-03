# Thread-safe Caching Web Proxy

## Team members
Jiuao Song, Zixuan Wang

## Related course material
- Multi-threading: in Part II, we use multi-threading to deal with concurrent requests.
- Socket

## Operating system and third-party libraries
- Ubuntu ?
- telnet ?

## Compilation

### Files
You can see there are 3 proxy-related cpp files in our folder. 

`proxy.cpp` implements a basic sequential proxy that handles HTTP GET requests. 

`proxy2.cpp` implements a concurrent server based on `proxy.cpp`, which spawns a new thread to handle each new connection. Our threads run in detached mode to avoid memory leaks. 

`proxy3.cpp` implements a cache to our proxy based on `proxy2.cpp` which stores web objects in memory. When the proxy receives an object from the server, it will cache it in the memory. If another client requests the same object from the same server, the proxy can simply resend the cache. Our cache also has a LRU eviction policy.

`ProxyConcurrentTest.sh` is our auto-testing script for testing concurrency.

`.sh` testing cache (?)

### Compile with `proxy.cpp`
```
g++ proxy.cpp csapp.cpp -o test -lpthread
```

This will create `test` in current working directory. 

Now, open a new terminal. With the following command, our proxy listens for connections on port 30308
```
linux> ./test 30308
```

### Compile with `proxy2.cpp`
```
g++ proxy2.cpp csapp.cpp -o test -lpthread
```

This will create `test` in current working directory.

Now, open a new terminal and run
```
linux> ./test 30308
```
Port 30308 is also hard-coded in the testing script.

### Compile with `proxy3.cpp`

```
g++ proxy3.cpp csapp.cpp -o test -lpthread
```

This will create `test` in current working directory.

Now, open a new terminal and run
```
linux> ./test 30308
```

Next, you can 
