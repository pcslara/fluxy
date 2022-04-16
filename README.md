# fluxy is a micro web server in C++

fluxy is a micro web server with REST Full support.

It is developed in c++20 a single .h and does not have any dependencies.

Just do it:
```cpp
#include "fluxy.h"
RouteStatus home( Request &req, Response &res ) {
    res.setData( R"( 
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

