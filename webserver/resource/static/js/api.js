//browseOpcua = async function(){ //restful style
//    rep = await fetch( 'http://' + window.location.hostname + ':8090/api/browse/' + '{\"namespaceindex\":0,\"identifiertype\":1,\"identifier\":1}')
//    data = await rep.text();
//    console.log(data)
//    return records
//}



const opcuaws = new WebSocket("ws://" + window.location.hostname + ":8090/api/websocket")
let rootNode = null;

opcuaws.onopen = (event) => {
    console.log("opcuaws open!")
};

opcuaws.onmessage = (event) => {
    console.log(event.data);
    let jsonstr = event.data.substring(0, event.data.length - 1);
    obj = JSON.parse(jsonstr)
    handleObject(obj);
    //$("#content").html(`<pre>${jsonstr}</pre>`)
};

opcuaws.onerror = (event) => {
    console.log("error")
}
opcuaws.onclose = (event) => {
    console.log("The connection has been closed successfully.");
};

function browseRoot(){
    nodeid = {}
    nodeid.namespaceindex = 0;
    nodeid.identifiertype = UA_NODEIDTYPE_NUMERIC;
    nodeid.identifier = UA_NS0ID_ROOTFOLDER;
    browseNode(nodeid)
}

function browseNode(nodeid){
    if(!nodeid) return
    let pkt = {}
    pkt.cmd = "browse"
    pkt.nodeid = nodeid
    opcuaws.send(JSON.stringify(pkt));
}

/* addMonitorAttribute(    {"namespaceindex": 1 , "identifiertype" : 3 ,"identifier" : "192.168.50.3" } ) */
function addMonitorAttributeVariable(nodeid){
    if(!nodeid) return
    let pkt = {}
    pkt.cmd = "addmonitorattribute"
    pkt.nodeid = nodeid
    pkt.attributeid = UA_ATTRIBUTEID_VALUE
    opcuaws.send(JSON.stringify(pkt));
}

function addMonitorAttributeEvent(nodeid){
    if(!nodeid) return
    let pkt = {}
    pkt.cmd = "addmonitorattribute"
    pkt.nodeid = nodeid
    pkt.attributeid = UA_ATTRIBUTEID_EVENTNOTIFIER
    opcuaws.send(JSON.stringify(pkt));
}


function delMonitorItem(subid,monid){
    if(!nodeid) return
    let pkt = {}
    pkt.cmd = "delmonitoritem"
    pkt.subid = subid
    pkt.monid = monid
    opcuaws.send(JSON.stringify(pkt));
}

function writeValue(){
    let pkt = buildObject();
    opcuaws.send(JSON.stringify(pkt));
}
