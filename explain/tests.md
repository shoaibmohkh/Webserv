

=========================================
2-basic cases:

1.1) curl -i --http1.0 http://127.0.0.1:8080/
ok 200
GET root index 

1.2)curl -i --http1.0 http://127.0.0.1:8080/files/a.txt
200 ok
file contents

1.3)curl -i --http1.0 http://127.0.0.1:8080/listing_dir/
200 ok
directory listing

1.4)curl -i --http1.0 http://127.0.0.1:8080/private_dir/
404 not found
html error page

1.5)dd if=/dev/zero of=big1g.bin bs=1M count=1024
    curl --http1.0 -v -F "file=@big1g.bin" http://127.0.0.1:8080/uploads

1.6)curl -i --http1.0 -X DELETE http://127.0.0.1:8080/delete_zone/protected.txt
204 no content

1.7)curl -i --http1.0 -X DELETE http://127.0.0.1:8080/delete_zone/nope.txt
404 not found

1.8)printf "BREW / HTTP/1.0\r\n\r\n" | nc 127.0.0.1 8080
405 method not allowed

1.9.1)curl -i --http1.0 -X POST http://127.0.0.1:8080/files/a.txt
411 length required

1.9.2)curl -i --http1.0 -X POST -H "Content-Length: 0" \
                                         http://127.0.0.1:8080/files/a.txt
405 method not allowed

2.1)curl -i --http1.0 http://127.0.0.1:9090/
200 OK

2.2)curl -i --http1.0 -X POST --data "X" http://127.0.0.1:9090/
405 method not allowd

2.3)dd if=/dev/zero of=2mb.bin bs=1M count=2
and add post to 9090 port then :
curl -i --http1.0 -F "file=@2mb.bin" http://127.0.0.1:9090/
413 payload too large


=========================================
3-CGI tests:

1.1)curl -i --http1.0 http://127.0.0.1:8080/cgi/test.py
run the python script, wait 10s and print cgi worked

1.2)curl -i --http1.0 "http://127.0.0.1:8080/cgi/q.py?v=1&x=2"
print the cgi query:
*string=v=1&x=2
*name=/cgi/q.py
*method=get

1.3)curl -i --http1.0 http://127.0.0.1:8080/cgi/loop.py
time out 

1.4)curl http://127.0.0.1:8080/cgi/hello.txt
403 Forbidden
html error page


==========================================
4-browser checkes:

1.1)http://127.0.0.1:8080/
index.html that in the root

1.2)http://127.0.0.1:8080/this_does_not_exist
page doesn't exist
404 page

1.3)http://127.0.0.1:8080/listing_dir/
file1.txt
file2.txt

1.4)http://127.0.0.1:8080/private_dir/
404 page no access

1.5)http://127.0.0.1:8080/files/index.html
index.html with css

1.6)http://127.0.0.1:8080/uploads/<uploaded_file>
open it without no errors


===========================================
5-ports:

1)same double ports should give us an error

2)multible ports should work as expect

3)multible ports should work on the browser

4)lunch the same config on two terminals should not work

5)different ports with different config should work on two terminals


=============================================
6-siege:

1.1)siege -b -c 25 -d 1 -r 200 http://127.0.0.1:8080/
-b → benchmark mode (no think time, max pressure)
-c 25 → 25 concurrent clients (reasonable, fair)
-d 1 → up to 1s delay before reconnect
-r 200 → each client makes 200 requests (≈ 5,000 total)

1.2)siege -b -c 20 -d 1 -r 1000 http://127.0.0.1:8080/