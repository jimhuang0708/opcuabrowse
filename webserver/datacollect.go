package main
import (
    "net"
    "os"
    "fmt"
//    "io"
//    "encoding/binary"
//    "encoding/json"
    "math/rand"
    "time"
//    "strconv"
//    "math"
    "bufio"
)

func random(max int , min int) int32 {
    return int32(rand.Intn( max-min ) + min )
}

func random16(max int , min int) int16 {
    return int16(rand.Intn( max-min) + min  )
}


func randomf(max int,min int ) float32 {
    return float32(rand.Intn(max-min) +min )
}

func randomboolean(max int , min int) int32 {
    return int32(rand.Intn(max-min) + min) % 2
}


func readCommand(con net.Conn , cmdCH chan string)(int){
    timeTrig := time.NewTicker( time.Millisecond * 1000 )
    select{
        case cmd := <-cmdCH:
 	    //jsondata, _ := json.Marshal(cmd)
            //fmt.Printf("send cmd %s\n",jsondata);
            if(cmd == "WSCLOSED"){
                return -1
            }
            con.Write( []byte(cmd) )
        case <-timeTrig.C:
            //fmt.Printf("TODO : ping\n")
            //if(ping failed){
            //    return -1;
            //}
            return 0
    }
    return 0
}

func readCommandLoop(conn net.Conn , cmdCH chan string){
    defer func(){
        conn.Close()
        //cmdCH <- string("WSCLOSED")
        fmt.Printf("readCommandLoop Close tcpsocket connection\n")
    }()

    for {
        if(readCommand(conn, cmdCH) < 0){
            return;
        }
    }
}



func ConnectOpcua(cmdCH chan string,resultCH chan string) {
    args := os.Args
    //servAddr := "192.168.51.229:7000"
    servAddr := gClientIP
    tcpAddr, err := net.ResolveTCPAddr("tcp", servAddr)
    if err != nil {
        println("ResolveTCPAddr failed:", err.Error())
        os.Exit(1)
    }

    conn, err := net.DialTCP("tcp", nil, tcpAddr)
    if err != nil {
        println("Dial failed:", err.Error())
        os.Exit(1)
    }
    fmt.Printf("Type of Args = %T\n", args)
    defer func(){
        conn.Close()
        resultCH <- "OPCUACLOSED"
        fmt.Printf("Close connection & exit handleConnection\n")
    }()

    go readCommandLoop(conn, cmdCH);


    conn.SetReadBuffer(1024 * 1024)
    conn.SetWriteBuffer(1024 * 1024)
    reader := bufio.NewReader(conn)
    for {
        conn.SetReadDeadline(time.Now().Add( time.Millisecond * 200 ))
        /*TODO: debug seems not work as expected,the json string still
          have chance will be divded into serveral segment*/
        jsonMsg , err := reader.ReadString(byte(0))
        if err != nil {
            if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
                // Handle timeout
            } else {
                // Handle other errors
                fmt.Printf("tcp readlen err : %v | msg : %s\n" ,err,jsonMsg)
                return
            }

        }
        if(len(jsonMsg) > 0 ){
            fmt.Printf("get %s\n",jsonMsg);
            resultCH <- jsonMsg
        }
    }
}


