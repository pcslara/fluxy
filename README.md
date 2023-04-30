# fluxy is a micro web server in C++

fluxy is a micro web server with REST Full support.

It is developed in c++20 a single .h and does not have any dependencies.

Just do it:
```cpp
#include "fluxy.h"
RouteStatus home( Request &req, Response &res ) {
    res.data( 
    R"( 
        <html>Fluxy is on!</html>
    )" );
    return RouteStatus::OK;
}
int main() {
    App app;
    app.get("/", home );
    app.start( 8080 );
}
```

Just go to the browser and access http://127.0.0.1:8080

You only need to know 3 classes:
- Request
- Response
- App

You can stack middlewares using a simple regex like
```cpp
#include "fluxy.h"
...
int main() {
    App app;
    app.middleware(".*", allRequests );
    app.middleware("/", homeMiddleware );
    app.middleware("/user(|/.*)", userMiddleware );
    app.post("/", home );
    app.middleware(".*", lastMiddleware );
    app.start( 8080 );
}
```


You can declare variables in url like

```cpp
#include "fluxy.h"
...
int main() {
    App app;
    app.get("/user/@id", user );
    app.start( 8080 );
}
```

You can consume others api's

```cpp
#include "fluxy.h"
int main(){
    try {
        Request req;
        Consume("http://127.0.0.1:8989").get("/", req , [](Response &res ) {
            cout << res.getRawString();
        }).join();      
    } catch( runtime_error& e) {
        cerr << e.what() << endl;
    }
    return 0;
}
```


