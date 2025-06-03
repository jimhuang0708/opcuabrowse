package main

import (
//    "net/http"
    "github.com/gin-gonic/gin"
    "os"
    "fmt"
//    "os/exec"
//    "time"
//    "bytes"
//    "io/ioutil"
//    "strconv"
    "io"
    "encoding/json"
//    "net"
    "log"
    "time"
    "github.com/gorilla/websocket"
)


type NODEID struct {
    NamespaceIndex int `json:"namespaceindex"`
    IdentifierType int `json:"identifiertype"`
    Identifier interface{} `json:"identifier"`
}

type CMDPKT struct {
    Cmd string `json:"cmd"`
    Nodeid *NODEID `json:"nodeid,omitempty"`
    Attributeid int `json:"attributeid,omitempty"`
    Subid int `json:"subid,omitempty"`
    Monid int `json:"monid,omitempty"`
}

type RESULTPKT struct {
    result string
}

var gClientIP string = ""

func doOpcuaBrowse(c *gin.Context) {
/*
    jsonStr := c.Param("nodeid")
    fmt.Printf("->%s\n",jsonStr);
    nodeid := NODEID{}
    err := json.Unmarshal([]byte(jsonStr), &nodeid)
    if(err != nil){
        fmt.Printf("Json parse fail @ doOpcuaBrowse\n");
    }
    fmt.Printf("nodeid : %v\n",nodeid);
    cmd := CMDPKT{ Cmd : "browse" , Nodeid : nodeid }
    e.cmdCH <- cmd
    c.IndentedJSON(http.StatusOK, "")
*/
}

func readResult(wsConn *WsConn , resultCH chan string)(int){
    timeTrig := time.NewTicker(time.Millisecond * 1000)
    select{
        case result := <-resultCH:
            //jsondata, _ := json.Marshal(cmd)
            if(result == "OPCUACLOSED"){
                return -1;
            }
            fmt.Printf("send result to ws %s\n",result);
            wsConn.WriteTextMessage( []byte(result) )
        case <-timeTrig.C:
            if err := wsConn.WriteMessage(websocket.PingMessage, nil); err != nil {
                log.Println("ws write ping failed !")
                log.Println(err)
		return -1
	    }
            return 0
    }
    return 0
}

func readResultLoop(wsConn *WsConn , resultCH chan string){
    defer func(){
        wsConn.Close()
        //cmdCH <- string("WSCLOSED")
        fmt.Printf("readResultLoop Close websocket connection\n")
    }()

    for {
        if(readResult(wsConn, resultCH) < 0){
            return;
        }
    }

}

func ConnectBrowser(wsConn *WsConn,cmdCH chan string,resultCH chan string) {
    defer func(){
        wsConn.Close()
        cmdCH <- string("WSCLOSED")
        fmt.Printf("ConnectBrowser Close websocket connection\n")
    }()
    go readResultLoop(wsConn , resultCH )
    wsConn.ws.SetReadLimit(1024*1024)
    for {
        //wsConn.SetReadDeadline(time.Now().Add( time.Millisecond * 100 ))
        /*
             websocket is always processing when all frame received(in chrome its 64KB)
             so basically don't worry about it will be split
        */
	//_, data, err := wsConn.ReadMessage()
       var cmd CMDPKT
       err :=  wsConn.ReadJson(&cmd)

	if err != nil {
            if(err == io.ErrUnexpectedEOF){
                continue
            }
            fmt.Println("Error reading json : ", err)
            break
	}

        data , _  := json.Marshal(cmd)
        if(len(data) > 0){
            fmt.Printf("-> %s\n",data);
            cmdCH <- string(data)
        }
    }
}

func doTcpProxy(wsConn *WsConn) {
    cmdch := make(chan string , 10)
    resultch := make(chan string , 10)
    go ConnectOpcua(cmdch,resultch)
    ConnectBrowser(wsConn,cmdch,resultch)
    return
}

func webSocketProxyHandler(c *gin.Context) {
	conn, err := UpgradeGinWsConn(c)
	if err != nil {
		log.Printf("WebSocket error: %v\n", err)
		return
	}
	defer conn.Close()
	doTcpProxy(conn)
}


func main() {
    args := os.Args
    if(len(args) != 2){
        fmt.Printf("required opcua client ip\n");
        return 
    }
    gClientIP = args[1]

    router := gin.Default()
    router.GET("/api/browse/:nodeid", doOpcuaBrowse)
    router.GET("/api/websocket", webSocketProxyHandler)

    router.Static("/site", "resource")
    router.Run(":8090")
}
