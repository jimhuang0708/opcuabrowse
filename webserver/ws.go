package main

import (
	"crypto/tls"
	"fmt"
	"net"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

var gWsUpgrader = &websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
	Subprotocols: []string{
		"binary",
	},
}

// =============================================================================
// WsConn
// =============================================================================
type WsConn struct {
	id         string
	addr       string
	ws         *websocket.Conn
}

func UpgradeGinWsConn(c *gin.Context) (*WsConn, error) {
	return UpgradeWsConn(c.Writer, c.Request)
}

func UpgradeWsConn(w http.ResponseWriter, r *http.Request) (*WsConn, error) {
	addr := GetHttpRemoteAddr(r)
	ws, err := gWsUpgrader.Upgrade(w, r, nil)
	if err != nil {
		return nil, fmt.Errorf("upgrade %s error: %v", addr, err)
	}

	return NewWsConn(ws, addr), nil
}

func GetHttpRemoteAddr(r *http.Request) string {
	addr := func() string {
		addr := r.Header.Get("X-Real-IP")
		if addr != "" {
			return addr
		}

		addr = r.Header.Get("X-Forwarded-For")
		if addr != "" {
			return addr
		}

		return r.RemoteAddr
	}()

	if strings.Contains(addr, ":") {
		host, _, err := net.SplitHostPort(addr)
		if err == nil {
			return host
		}
	}

	return addr
}

func ConnectWsWithProtocol(wsUrl string, protocol string) (*WsConn, error) {
	u, err := url.Parse(wsUrl)
	if err != nil {
		return nil, err
	}

	wsHeaders := http.Header{
		// "Origin": {"http://local.host:80"},
		// "Sec-WebSocket-Extensions": {"permessage-deflate; client_max_window_bits, x-webkit-deflate-frame"},
	}
	if protocol != "" {
		wsHeaders.Add("Sec-WebSocket-Protocol", protocol)
	}

	dialer := &websocket.Dialer{
		ReadBufferSize:   16 * 1024,
		WriteBufferSize:  16 * 1024,
		HandshakeTimeout: 30 * time.Second,
		TLSClientConfig:  &tls.Config{InsecureSkipVerify: true},
	}

	ws, _, err := dialer.Dial(wsUrl, wsHeaders)
	if err != nil {
		return nil, err
	}

	return NewWsConn(ws, u.Host), nil
}

func ConnectWs(wsUrl string) (*WsConn, error) {
	return ConnectWsWithProtocol(wsUrl, "")
}

func NewWsConn(ws *websocket.Conn, addr string) *WsConn {
	conn := &WsConn{}
	conn.addr = addr
	conn.ws = ws

	return conn
}

func (conn *WsConn) Close() {
	conn.ws.Close()
}

func (conn *WsConn) Socket() *websocket.Conn {
	return conn.ws
}

func (conn *WsConn) GetAddr() string {
	return conn.addr
}

func (conn *WsConn) SetWriteDeadline(t time.Time) error {
	return conn.ws.SetWriteDeadline(t)
}

func (conn *WsConn) SetReadDeadline(t time.Time) error {
	return conn.ws.SetReadDeadline(t)
}

func IsNetTimeoutError(err error) bool {
	e, ok := err.(net.Error)
	if !ok {
		return false
	}

	return e.Timeout()
}

func (conn *WsConn) ReadJson(v interface{}) error {

	return conn.ws.ReadJSON(v)
}

func (conn *WsConn) WriteJson(v interface{}) error {
	err := conn.ws.WriteJSON(v)
	return err
}

func (conn *WsConn) ReadMessage() (messageType int, p []byte, err error) {

	return conn.ws.ReadMessage()
}

func (conn *WsConn) WriteMessage(messageType int, p []byte) error {
	err := conn.ws.WriteMessage(messageType, p)
	return err
}

func (conn *WsConn) WriteTextMessage(p []byte) error {
	err := conn.ws.WriteMessage(websocket.TextMessage, p)
	return err
}

func (conn *WsConn) WriteBinaryMessage(p []byte) error {
	err := conn.ws.WriteMessage(websocket.BinaryMessage, p)
	return err
}
