#ifndef __FLUXY_H__
#define __FLUXY_H__

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <list>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <regex>
#include <sstream>
#include <ostream>
#include <fstream>
#include <chrono>
#include <ctime>  
#include <iomanip>
#include <cerrno>
#include <filesystem>
#include <thread>


using namespace std;
using namespace std::chrono;

#define h(...) #__VA_ARGS__
#define QUEUE_SIZE 1000000
#define VERSION     "1.0.1"
#define MAX_REQ_SIZE 5242880



map< unsigned long int, list<void *>> garbage = map< unsigned long int, list<void *>>();

map< string, string > acceptedFiles = {
   {"html", "text/html"},
   {"js", "application/javascript"},
   {"htm", "text/html"},
   {"png", "image/png"},
   {"jpg", "image/jpeg"},
   {"jpeg", "image/jpeg"},
   {"gif", "image/gif"},
   {"gz", "application/gzip"},
   {"json", "application/json"},
   {"pdf", "application/pdf"},
   {"svg", "image/svg+xml"},
   {"xml", "application/xml"},
   {"webp", "image/webp"}, 
   {"zip", "application/zip"}
};

pthread_mutex_t __mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t __log_mutex = PTHREAD_MUTEX_INITIALIZER;

vector<string> split (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;
    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }
    res.push_back (s.substr (pos_start));
    return res;
}
void replaceAll(std::string & data, std::string toSearch, std::string replaceStr) {
    size_t pos = data.find(toSearch);
    while( pos != std::string::npos){
        data.replace(pos, toSearch.size(), replaceStr);
        pos =data.find(toSearch, pos + replaceStr.size());
    }
}
bool getDataFromFile(string urlPath, string& chunkData) {
    try {
        std::stringstream stream;
        if( urlPath[0] != '/' ) {
            urlPath = "/" + urlPath;
        }
        std::string localPath = string(".") + urlPath;


        
        filesystem::path absolute = filesystem::absolute(filesystem::path(localPath));
        filesystem::path pwd = filesystem::current_path();
        std::ifstream file(localPath.c_str());
        if( !file.good() )   return false;
        file.seekg(0, std::ifstream::beg);
        ifstream fin(localPath, ios::binary);
        stream << fin.rdbuf();
        chunkData = stream.str();
        file.close();
        return true;
    } catch(...) {
        return false;
    }
} 


bool isAcceptedFile( string extension ) {
    return ( acceptedFiles.find( extension ) != acceptedFiles.end() );
}

typedef enum __Method{
        GET  = 0,
        POST = 1,
        PUT  = 2,
        DELETE = 3,
        PATCH = 4,
        UNDEF = 5,
        ALL = 6
} Method;


string methodToString( Method m ) {
    if( m == Method::GET )   return "GET";
    if( m == Method::POST)   return "POST";
    if( Method::ALL )        return "ALL";
    if( m == Method::PUT )   return "PUT";
    if( m == Method::DELETE) return "DELETE";
    if( m == Method::PATCH)  return "PATCH";
    return "UNDEF";    
}


Method stringToMethod( string method ) {
    if( method == "GET" ) return Method::GET;
    else if ( method == "POST" ) return Method::POST;
    else if ( method == "PUT" ) return Method::PUT;
    else if ( method == "PATCH" ) return Method::PATCH;
    else if ( method == "DELETE" ) return Method::DELETE;
    return Method::UNDEF;
}

typedef enum __RouteStatus{
        OK,
        ERROR
} RouteStatus;

enum Color {
    FG_BLACK    = 30,
    FG_RED      = 91,
    FG_GREEN    = 32,
    FG_BLUE     = 94,
    FG_MAGENTA  = 95,
    FG_WHITE    = 97,
    FG_YELLOW   = 93,
    FG_DEFAULT  = 39,
    BG_BLACK    = 40,
    BG_RED      = 101,
    BG_GREEN    = 42,
    BG_BLUE     = 104,
    BG_WHITE    = 107,
    BG_YELLOW   = 103,
    BG_MAGENTA  = 105,
    BG_DEFAULT  = 49
};

class Modifier {
    Color code;
public:
    Modifier(Color pCode) : code(pCode) {}
    friend std::ostream&
    operator<<(std::ostream& os, const Modifier& mod) {
        return os << "\033[" << mod.code << "m";
    }
};

#define LOG_I(...) Log::info( __LINE__, __FILE__, "INFO  " ,Modifier(Color::BG_BLACK),Modifier(Color::FG_GREEN), __VA_ARGS__ )
#define LOG_W(...) Log::info( __LINE__, __FILE__, "WARN  " ,Modifier(Color::BG_BLACK),Modifier(Color::FG_MAGENTA), __VA_ARGS__ )
#define LOG_E(...) Log::info( __LINE__, __FILE__, "ERR0R " ,Modifier(Color::BG_BLACK),Modifier(Color::FG_RED), __VA_ARGS__ ) 
#define LOG_S(...) Log::info( __LINE__, __FILE__, "SERVER" ,Modifier(Color::BG_BLACK),Modifier(Color::FG_BLUE), __VA_ARGS__ )


class Log {
public:
    static string now() {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
        return oss.str();
    }
    
    static uint64_t ms() {
        
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    template<class... Args>
    static void info (int lineno, const string& filename, string severe, Modifier bgcolor, Modifier fgcolor,  Args... args ) {
        pthread_mutex_lock( &__log_mutex );
        (cout << bgcolor << fgcolor << "[" << now() << "."<<std::setfill('0') << std::setw(3) << (Log::ms() % 1000) << "] " << severe << " " << "{"<< filename << ":" << lineno << "}" << " " << Modifier(FG_YELLOW) << ... << args) << Modifier(BG_DEFAULT) << Modifier(FG_DEFAULT) << endl;
        pthread_mutex_unlock( &__log_mutex );
    }
};


namespace fluxy {
    string format(const string& filename, map<string, string> vars ) {
        string file = "";
        cout << "Filename: " << filename << endl;
        if( getDataFromFile(filename, file) ) {
            for( auto& [key, value] : vars ) {
                replaceAll( file, "{{"+key+"}}", value );
            }
        }
        cout << "File: " << file << endl;
        try {
            smatch match;
            regex r("\\{\\{[a-zA-Z_][a-zA-Z0-9]*\\}\\}");
            while (regex_search(file, match, r)) {
                LOG_W( "Variable",  match.str(0), " was not assigned in file ", filename );
                replaceAll( file, match.str(0), "" );
            }
        } catch( exception& e) {
            cout << ("format() error: ") << e.what() << endl;
        }
        return file;
    }
}

class Response {
private:
    int httpStatus;
    string protocol;
    map<string, string> headers;
    string payload;

    
    void setDefaultHeaderValue() {
        headers["Connection"] = "close";
        headers["Content-Type"] = "text/html";
        httpStatus = 200;        
    }

public:

    Response() {
        setDefaultHeaderValue();
    }

    Response( string rawString ) {
        try {
            vector<string> response = split(rawString, "\n");
            if( response.size() <=  2 ) {
                LOG_E("Respose parse error");
                return;
            }
            vector<string> firstLine = split (response[0], " ");
            if( firstLine.size() < 2 ) {
                LOG_E("Response first line parse error");
                return;
            }

            httpStatus = atoi( firstLine[1].c_str() );
            protocol = firstLine[0];
            int i = 1;
            while(  response[i] != "" && response[i] != "\r" && i < response.size() ) {
                vector<string> header = split( response[i], ": " );
                if( header.size() == 2 ) {
                    headers[header[0]] = header[1];
                }  
                i++;
            }
            i++;
            payload = "";
            while( i < response.size() ) {
                payload += response[i] + "\n";
                i++;
            }

        } catch( exception& e ) {
            LOG_E( "Bad response raw string: ", e.what() );
        }
    }
    
    string getRawString() {
        string raw = "";
        raw += "HTTP/1.1 " + to_string( httpStatus ) +  "\n";
        for (auto& [key, value]: headers) {
            raw += key + ": " +  value + "\n";
        } 
        if( !headers.contains("Content-Length") )
            raw += "Content-Length: " + to_string( payload.size() ) + "\n";
        raw += "\n";
        raw += payload;
        return raw;
    }
    
    Response& setStatus( int httpStatus ) { this->httpStatus = httpStatus; return *this; }
    Response& setData( const string& payload  ) { this->payload = payload; return *this;}
    Response& setData( const stringstream& ss  ) { this->payload = ss.str(); return *this;}
    Response& setData( const string& filename, map<string,string> vars ) { 
        this->payload = fluxy::format(filename, vars );    
        return *this;
    }
    Response& addHeader( string key, string value ) {
        headers[key] = value;
        return *this;
    }
};



class Request {
private:
    string protocol;
    string uri;
    Method method;
    map<string, string> headers;
    string payload;
    map<string, string> parameters;
    string route; // uri + parametes

    void _parse( string rawString ) {
        try {
            vector<string> request = split(rawString, "\n");
            if( request.size() <=  2 ) {
                LOG_E("Request parse error");
                return;
            }
            LOG_I("Request: ", request[0] );
            vector<string> firstLine = split (request[0], " ");
            if( firstLine.size() != 3 ) {
                LOG_E("Request first line parse error");
                return;
            }
            method = stringToMethod( firstLine[0] );
            route = firstLine[1];
            protocol = firstLine[2];
            int i = 1;
            while(  request[i] != "" && request[i] != "\r" && i < request.size() ) {
                vector<string> header = split( request[i], ": " );
                if( header.size() == 2 ) {
                    headers[header[0]] = header[1];
                }  
                i++;
            }
            i++;
            payload = "";
            while( i < request.size() ) {
                payload += request[i] + "\n";
                i++;
            }
            vector<string> uri_params = split(route, "?");
            if( uri_params.size() >= 2 ) {
                vector<string> params = split(uri_params[1], "&");
                for( auto & p : params ) {
                    vector<string> param = split( p, "=" );
                    if( param.size() >= 2 ) {
                        parameters[param[0]] = param[1];
                    }
                }
                uri = uri_params[0];
            } else {
                uri = route;
            }

        } catch( exception& e ) {
            LOG_E( "Bad request raw string: ", e.what() );
        }
    }
    
public:    
    Request() {}
    Request( string rawString ) {
        _parse( rawString );
    }
    void setRawString( string rawString ) {
        _parse( rawString );
    }
    string& operator[](const string& key ) {
        return parameters[key];
    }    

    Request& addHeader( string key, string value ) {
        headers[key] = value;
        return *this;
    }    
    
    string getBody() {
        return payload;
    }
    
    Method getMethod() { 
        return method;
    }

    void setMethod( Method method ) {
        this->method = method;
    }
    
    Request& setData( string body ) {
        this->payload = body;
        return *this;
    }
    
    string getUri() {
        return uri;
    }

    void setUri( string uri ) {
        this->uri = uri;
    }
    
    string getRawString(){
        stringstream ss;
        if( parameters.size() > 0 ){
            uri += "?";
            int i = 0;
            for( auto&[key, value] :parameters ){
                uri += key + "=" + value + ((i != parameters.size() - 1) ? "&" : "");
                i++;
            }
        }
        ss << methodToString( method ) << " " << uri << " HTTP/1.1" << endl;
        for( auto& [key, value] : headers ) {
            ss << key << ": " << value << endl;
        }
        if( !headers.contains("Content-Length") )
           ss << "Content-Length: " + to_string( payload.size() ) << endl;
        
        
        ss << endl;

        ss << payload;
        return ss.str();
    }

    void print() {
        cout << "Method: " << method << endl;
        cout << "URI: " << uri << endl;
        cout << "Protocol: " << protocol << endl;
        cout << "HEADERS: " << endl;
        for (auto& [key, value]: headers) {
            std::cout << key << ": " << value << std::endl;
        }
        cout << "PAYLOAD: " << endl;
        cout << payload << endl;
    }    
};

class Route {
private:
    string route;
    RouteStatus (*callback)( Request&, Response& );
    Method method;
    list<string> routeVariables;
    string routeRegex;
public:
    Route() {}
    Route( string route, RouteStatus (*callback)( Request&, Response& ), Method method ) : route(route), callback(callback), method(method) { 
        int pos = 0;
        int posRegex = 0;
        routeRegex = route;
        int i = 0;
        do {
            string routeVariable = "";
            
            if( (pos = routeRegex.find("@")) != string::npos ) {
                int barPos = routeRegex.find("/", pos );
                if( barPos != string::npos ){
                    routeVariable = routeRegex.substr( pos + 1, barPos-pos-1 ); 
                } else 
                    routeVariable = routeRegex.substr( pos + 1 );
                
                replaceAll(routeRegex, "@" + routeVariable, "[^/]+");

                if( routeVariable == "" ) {
                    LOG_W( "No route variable on route: ", route); 
                }
                routeVariables.push_back( routeVariable );
            } 
            i++;
            if( i > 32 )
                break;

        } while( pos != string::npos );
    }
    const RouteStatus call( Request& req, Response& res ) {
        return callback( req, res );
    }
    const string getRoute() { return route; }
    const Method getMethod() { return method; }
    const string getRouteRegex() { return routeRegex; }
    const list<string> getRouteVariables() { return routeVariables; }
    const map<string, string> extractVariableValue( string uri ) {
        if( route.find("@") == string::npos ) map<string, string>();
        auto splitUri = split( uri, "/" );
        auto splitRoute = split( route, "/" );
        map<string, string> ret;
        for( int i = 0; i < splitRoute.size(); i++ ) {
            if( splitRoute[i][0] == '@' ) {
                ret[splitRoute[i].substr(1)] = splitUri[i];
            }
        }
        return ret;
    }
};

typedef struct __thread_param {
    bool * alive;
    bool * finished;
    unsigned long int identifier;
    int sockfd; 
    Route routeOnNotFound;
    list<Route> routes;
}thread_param;

void garbage_register( unsigned long int identifier, void * ptr ) {
    pthread_mutex_lock( &__mutex );
    garbage[identifier].push_back( ptr );
    pthread_mutex_unlock( &__mutex );
}


void garbage_release( unsigned long int identifier ) {
    pthread_mutex_lock( &__mutex );
    for( void * ptr : garbage[identifier] ) {
        if( ptr != NULL) {
            free( ptr );
            ptr = NULL;
        }
    }
    garbage.erase( identifier );
    pthread_mutex_unlock( &__mutex );
}



void *  thread_response( void * p );



class ThreadResponse {
public:
    bool  finished;
    bool  alive;
    int   sockfd;
    Route routeOnNotFound;
    list< Route > routes;
    unsigned long int startTime;
    pthread_t localThread;
    unsigned long int identifier;

    ThreadResponse() { finished = false; alive = false; startTime = 0; }

    //~ThreadResponse() {
    //    finalize();
    //}

    ThreadResponse(int sockfd, const Route& routeOnNotFound, const list<Route>& routes) : sockfd(sockfd), routeOnNotFound(routeOnNotFound),routes(routes) { finished= false; alive = false; startTime = 0; }
    const bool isAlive() { return alive; }
    const bool isFinished() { return finished; }
    void run() {        
        identifier = rand();
        startTime = time(NULL);
        pthread_create( &localThread, NULL, thread_response, (void *) this );
    }

    void setData( int sockfd, const Route& routeOnNotFound, const list< Route >& routes ) {
        this->sockfd = sockfd;
        this->routeOnNotFound = routeOnNotFound;
        this->routes = routes;
    }
    
    const unsigned long int getStartTime() { return startTime; }
    void finalize() {
        try {
            if( alive ) {
                pthread_cancel( localThread  );
                pthread_join( localThread, NULL );
                close( sockfd );
                alive = false;
                finished = false;
                routes.clear();
                startTime = 0;
            }
        } catch( exception e ) {
            LOG_E("Exception: ", e.what() );
        }
        garbage_release(identifier);
    }

    void clear() {
        alive = false;
        finished = false;
        routes.resize(0);
        routes.clear();
        startTime = 0;
        garbage_release(identifier);
    }

    void join() {
        if( finished ) {
            pthread_join( localThread, NULL );
            alive = false;
            finished = false;
            routes.clear();
            startTime = 0;
        }
    }
};
void *  thread_response( void * p ) {
        //pthread_mutex_lock(&__mutex );
        ThreadResponse * this_thread = (ThreadResponse *)p;
        this_thread->alive = true;
        this_thread->finished = false;
        
        try {        
            Request  req;
            Response res;
            int rcvd;
            int BUF_SIZE = MAX_REQ_SIZE;

            char * buf =  (char *) malloc( BUF_SIZE );
            garbage_register( this_thread->identifier, buf );
            
            /*
            struct timeval tv;
            tv.tv_sec = 4;
            tv.tv_usec = 0;

            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof( tv )) < 0 ) {
                LOG_E( "Time out: ",  strerror(errno)  );
                goto RELEASE_MEM;
            }
            */

            rcvd = recv( this_thread->sockfd, buf, BUF_SIZE, 0 );
            if (rcvd < 0) { // receive error
                LOG_E( "recv() returns: ", rcvd );
                goto RELEASE_MEM;
            } else if (rcvd == 0) { // receive socket closed
                LOG_E( "recv() returns: ", rcvd );
                goto RELEASE_MEM;
            } else if( rcvd >= MAX_REQ_SIZE ) {
                LOG_E( "Request size is too large. See MAX_REQ_SIZE variable." );
                goto RELEASE_MEM;
            } else {
                req.setRawString( string(buf) );
          //      free( buf );
          //      buf = 0;
            }
            bool routeFound = false;
            for (auto route : this_thread->routes) {
                try {
                    regex str_expr( route.getRouteRegex() );
                    string reqUri = req.getUri();
                    if( regex_match (reqUri,str_expr) ) {
                        if( req.getMethod() == route.getMethod() ||  
                            route.getMethod() == Method::ALL ) {
                            if( route.getRouteVariables().size() != 0 ) {
                                map<string, string> values = route.extractVariableValue( req.getUri() );
                                for( auto& [key, value] : values ) {
                                    req[key] = value;
                                }
                            }    
                            if( req.getMethod() == route.getMethod() ) { 
                                routeFound = true;
                            }
                            route.call( req, res );
                        } 
                    } 
                } catch( exception& e) {
                    LOG_E( e.what() );
                }
            }
            vector<string> uri_split = split( req.getUri(), "." );
            if( routeFound == false && uri_split.size() >= 2 ) {
                
                string uri = req.getUri();
                string extension = uri_split[uri_split.size()-1];
                if( isAcceptedFile( extension ) ) {
                    string chunk;
                    res.addHeader( "Content-Type", acceptedFiles[extension] );
                    if( getDataFromFile( uri, chunk ) ) {
                        res.setData( chunk );
                        routeFound = true;
                    }
                }
            }
            if( !routeFound ) {
                LOG_I("Route not found: ", methodToString( req.getMethod() ), " ", req.getUri());
                if( this_thread->routeOnNotFound.getRoute() == "" )
                    res.setStatus(404);
                else 
                    this_thread->routeOnNotFound.call( req, res );
            }
            send( this_thread->sockfd, res.getRawString().c_str(), res.getRawString().size(),  MSG_DONTWAIT );
        } catch( exception& e ) {
            LOG_E( e.what() );
        } 
        this_thread->alive = false;
        this_thread->finished = true;
        //pthread_mutex_unlock(&__mutex );
        RELEASE_MEM:
        close( this_thread->sockfd );
        garbage_release( this_thread->identifier );
        return NULL;
}
class ThreadPool {
public:
    int max_threads;
    int timeout;
    vector< ThreadResponse > threads;
    jthread timeoutMonitor;

    void timeoutCollector() {
        while( true ) {
            for( int i = 0; i < max_threads; i++ ) {
                if( threads[i].isAlive() &&
                    threads[i].getStartTime() != 0 && 
                    time(NULL) - threads[i].getStartTime() > timeout ) {
                    LOG_I("Response timeout, finalizing thread: ", i ); 
                    threads[i].finalize();
                }
            }
            sleep( 16 );
        }
    }
    ThreadPool( int max_threads = 64, int timeout = 2 ) : max_threads(max_threads ), timeout( timeout ) {
        threads = vector< ThreadResponse >( max_threads );
        timeoutMonitor = jthread( &ThreadPool::timeoutCollector, this );
        timeoutMonitor.detach();
    }
    void asyncResponse( int sockfd, const Route& routeOnNotFound, const list< Route >& routes ) {
        int i = getNextFreeSlot();
        threads[i].finalize();
        threads[i].setData(sockfd, routeOnNotFound, routes );
        threads[i].run();
    }
    int getNextFreeSlot() {
        do {
            for( int i = 0; i < max_threads; i++ ) {
                if( !threads[i].isAlive() ) {
                    threads[i].join();
                    return i; 
                }
            }
            usleep( 1000 );
        } while( true );
        return 0;    
    }
};
class AsyncResponse {
private:
    pthread_t thread;
public:
    AsyncResponse( pthread_t thread ) : thread(thread) {}
    void join() {
        if( thread != 0 )
            pthread_join(thread, NULL );
    }
    void terminate() {
        if( thread != 0 ) {
            pthread_cancel( thread );
            join();
        }
    }
};  

class Consume {
private:
    string host;
public:
    Consume( string host ) : host(host){}
    int connect( const string & uri, string &path, string &query ) {
        string route = host + uri;
        regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
        cmatch what;
        if(regex_match(route.c_str(), what, ex)) {
            string protocol = string(what[1].first, what[1].second);
            string domain   = string(what[2].first, what[2].second);
            string port     = string(what[3].first, what[3].second);
            path     = string(what[4].first, what[4].second);
            query    = string(what[5].first, what[5].second);
            int sock = 0;
            struct sockaddr_in serv_addr;
    
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                LOG_E("Socket creation error: ",sock );
                return -1;
            }
            struct timeval tv;
            tv.tv_sec = 16;
            tv.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof( tv ));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons( atoi( port.c_str()) );
            if (inet_pton(AF_INET, domain.c_str(), &serv_addr.sin_addr) <= 0) {
                LOG_E("Invalid address/Address not supported");
                return -1;
            }
            if (::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
                LOG_E("Connection failed");
                return -1;
            }
            return sock;
        }
        return 0;
    }
    static void send( int sockfd, const string & str   ){
        if( ::send(sockfd, str.c_str(), str.size(), MSG_DONTWAIT ) < 0 ) {
            LOG_E("Consume send error");
        }
    }
    static int recv( int sockfd, string &str  ) {
        char * buf =  (char *) malloc( MAX_REQ_SIZE );
        if( ::recv(sockfd, buf, MAX_REQ_SIZE, 0 ) < 0 ) {
            LOG_E("Consume recv error");
            return -1;
        }
        str = string( buf );
        free( buf );
        return 0;
    }

    void asyncRequest( Request req, void (*callback)( Response& ), int sockfd ) {
        Consume::send( sockfd,  req.getRawString() );
        string response;
        if( Consume::recv( sockfd, response ) == 0 ) {
            Response res( response );
            callback( res );
            close( sockfd );
        }
    }

    AsyncResponse get(  const string & uri, Request &req, void (*callback)( Response& ) ) {
        return fetch( Method::GET, uri, req, callback );
    }
  
    AsyncResponse fetch( Method method, const string & uri, Request &req, void (*callback)( Response& ) ) {
        pthread_t th = 0;
        try {
            string path;
            string query;
            int sockfd = Consume::connect(uri, path, query );
            if( sockfd >= 0 ) {
                if( path == "" ) path = "/";
                if( query != "" ) query = "?" + query;
                req.setUri( path + query );
                req.setMethod( method );
                jthread t( &Consume::asyncRequest, this, req, callback, sockfd );
                th = t.native_handle();
                return AsyncResponse(th);
            }
        } catch( exception & e ) {
            LOG_E("Fetch exception: ", e.what());
        }
        return AsyncResponse(0);
    }
};

class App {
private:    
    ThreadPool threadPool;
    int serverSocket;
    list< Route > routes;
    Route routeOnNotFound;
    int numThreads      = 32;
    int timeouResponse  = 16; // Seconds

    bool initialize( int port ) {
        int yes = 1;
        socklen_t addr_size;
        struct addrinfo hints, *res, *p;
        struct sockaddr_in serverAddr;

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        /*
        struct timeval tv;
        tv.tv_sec = 25;
        tv.tv_usec = 0;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof( tv )) < 0 ) {
            LOG_E( "setsockopt() error: ",  strerror(errno)  );
            close(serverSocket);
            return false;
        }*/

        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0 ) {
            LOG_E( "setsockopt() error: ",  strerror(errno)  );
            close(serverSocket);
            return false;
        }

        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if( bind(serverSocket,
            (struct sockaddr*)&serverAddr,
            sizeof(serverAddr)) < 0 ) {
            LOG_E( "bind() error: ",  strerror(errno)  );
            close(serverSocket);
            return false;       
        }
        if (listen( serverSocket, 128 ) != 0) {
            LOG_E( "listen() error: ",  strerror(errno)  );
            close(serverSocket);
            return false;
        }
        return true;
    }    
public:

    

    App() {

    }
    
    void addRoute( const Route & route ) {
        routes.push_back( route );
    }
    void middleware( const string & route, RouteStatus (*callback)( Request&, Response& ) ) {
        routes.push_back( Route( route, callback, Method::ALL ) );
    }
    void get( const string & route, RouteStatus (*callback)( Request&, Response& ) ) {
        routes.push_back( Route( route, callback, Method::GET ) );
    }
    void post( const string & route, RouteStatus (*callback)( Request&, Response& ) ) {
        routes.push_back( Route( route, callback, Method::POST ) );
    }
    void put( const string & route, RouteStatus (*callback)( Request&, Response& ) ) {
        routes.push_back( Route( route, callback, Method::PUT ) );
    }
    void patch( const string & route, RouteStatus (*callback)( Request&, Response& ) ) {
        routes.push_back( Route( route, callback, Method::PATCH ) );
    }
    void del( const string & route, RouteStatus (*callback)( Request&, Response& ) ) {
        routes.push_back( Route( route, callback, Method::DELETE ) );
    }
    void onRouteNotFound( RouteStatus (*callback)( Request&, Response& ) ) {
        routeOnNotFound = Route( "*", callback, Method::ALL );
    }
    void start( int port ) {
        try {
            struct sockaddr_in clientaddr;
            socklen_t addrlen = 0;
            if( !initialize( port ) )
                return;
            LOG_S( "Fluxy (v", VERSION,") web server started at http://127.0.0.1:", port );
            while( true ) {
                int sockfd = accept(serverSocket, (struct sockaddr *)&clientaddr, &addrlen);
                if( sockfd > 0 ) {
                    threadPool.asyncResponse( sockfd, routeOnNotFound, routes );
                } else {
                    LOG_E("Accept error: ", strerror(errno) );
                } 
            }
        } catch(exception e ) {
            LOG_E("Exception: ", e.what() );
        }
    }
};

#endif