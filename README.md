# Opcuabrowse
a practice project for learning opcua client

# Prerequirements
Tool : gcc / golang <br>
apt-get install libcurl4-openssl-dev libcjson-dev <br>

# Install
make;<br>
this will generate opcuaclient/opcuaserver <br>
cd webserver;go build -buildvcs=false . <br>
this will generate main

# Run
./opcuaserver<br>
./opcuaclient opc.tcp://127.0.0.1:14840<br>
./main  127.0.0.1:7000<br>
browser connect  to <br>
http://127.0.0.1:8090/site/client.html<br>


## License

Licensed under the [MIT license](https://github.com/shadcn/ui/blob/main/LICENSE.md).
