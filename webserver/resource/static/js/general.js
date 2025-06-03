
function cmpNodeID(n1,n2){
    if( (n1.identifier == n2.identifier)
      && (n1.namespaceindex == n2.namespaceindex)
      && (n1.identifiertype == n2.identifiertype) ){
        return true
    }
    return false
}

function handleObject(obj){
    if(obj.answertype == "RESPONSE"){
        handleObjectResonse(obj)
    } else if(obj.answertype == "ASYNC_VARIABLE"){
        handleObjectAsyncVariable(obj)
    } else if(obj.answertype == "ASYNC_EVENT"){
        handleObjectAsyncEvent(obj)
    }

}

function addOrUpdateNode(data,parentNode){
    let childNode = null;
    nodes = parentNode.findAll((n) => {
        return (n.data.idx == data.idx)
    });
    if(nodes.length > 1){
        alert("idx should be unique");
    }
    if(data.idx == "nodeclass"){
        data.value = getNodeClassDisplay(data.value)
    }


    if(nodes.length == 0){
        childNode = parentNode.addChildren(data,"")
    } else {
        childNode = nodes[0]
        childNode.data = data
        childNode.update("data")
    }
    return childNode;
}

function rip(parentNode,obj){
    if(Array.isArray(obj)){
        for(let i = 0 ; i < obj.length ; i++){
            if(typeof(obj[i]) == "object"){
                let childNode = addOrUpdateNode({title : "[" + i + "]", type:"structure"  , idx : parentNode.data.key + "_" + i},parentNode)
                rip(childNode,obj[i]);
            } else {
                addOrUpdateNode({title : "[" + i + "]" , type:"instance" , value : obj[i] , idx : parentNode.data.key + "_" + i },parentNode)
                //parentNode.addChildren({title : "[" + i + "]" , type:"instance" , value : obj[i] , idx : parentNode.data.key + "_" + i },"")
            }
        }
    } else {
        for (const [key, value] of Object.entries(obj)) {
            if(key == "references" || key == "answertype"){
                continue;
            }
            console.log(`${key}: ${value}`);
            if(typeof(value) == "object"){
                if(value == null){//null is an object
                    addOrUpdateNode({title : key , type:"instance" , value : "null" , idx:key},parentNode)
                    //parentNode.addChildren({title : key , type:"instance" , value : "null" , idx : key },"")
                } else {
                    let childNode =  addOrUpdateNode({title : key , type:"structure" ,idx : key  },parentNode)
                    rip(childNode,value);
                }
            } else {
                 addOrUpdateNode({title : key , type:"instance" , value : value ,idx:key},parentNode)
                //parentNode.addChildren({title : key , type:"instance" , value : value , idx : key},"")
            }
        }
    }
}

function handleObjectAsyncVariable(obj){
    mi = {}
    key = "subid:" + obj.subid + " monid:" + obj.monid
    mi[key] = obj
    rip( opcuaMonitorItem , mi);
    nodes = opcuaMonitorItem.findAll((n) => {
        return (n.data.idx == key)
    });
    nodes[0].data.subid = obj.subid
    nodes[0].data.monid = obj.monid
    //opcuaMonitorItem.expandAll()
}

function handleObjectAsyncEvent(obj){
    mi = {}
    key = "subid:" + obj.subid + " monid:" + obj.monid
    mi[key] = obj
    rip( opcuaMonitorEvent , mi);
    nodes = opcuaMonitorEvent.findAll((n) => {
        return (n.data.idx == key)
    });
    nodes[0].data.subid = obj.subid
    nodes[0].data.monid = obj.monid
    //opcuaMonitorItem.expandAll()
}


function handleObjectResonse(obj){
    let nodes = [];
    if(rootNode == null){
        obj.title = obj.displayname.text;
        rootNode = window.opcuaTree.addChildren( obj ,"")
        nodes.push( rootNode );
    } else {
        nodes = rootNode.findAll((n) => {
            return cmpNodeID(n.data.nodeid , obj.nodeid)
        }); //should be unique
    }
    let expanded = nodes[0].expanded;
    nodes[0].removeChildren();

    hasTypedefNode = {}
    hasTypedefNode.namespaceindex = 0
    hasTypedefNode.identifiertype = UA_NODEIDTYPE_NUMERIC;
    hasTypedefNode.identifier = UA_NS0ID_HASTYPEDEFINITION;

    hasNotifierNode = {}
    hasNotifierNode.namespaceindex = 0
    hasNotifierNode.identifiertype = UA_NODEIDTYPE_NUMERIC;
    hasNotifierNode.identifier = UA_NS0ID_HASNOTIFIER

    hasEventSourceNode = {}
    hasEventSourceNode.namespaceindex = 0
    hasEventSourceNode.identifiertype = UA_NODEIDTYPE_NUMERIC;
    hasEventSourceNode.identifier = UA_NS0ID_HASEVENTSOURCE

    for (const [key, value] of Object.entries(obj)) {
        nodes[0].data[key] = value
        //console.log(`${key}: ${value}`);
    }

    for(let i = 0 ; i < obj.references.length ; i++){
        if( cmpNodeID(hasTypedefNode,obj.references[i].reftype)){
            continue;
        }
        if( cmpNodeID(hasNotifierNode,obj.references[i].reftype)){
            continue;
        }
        if( cmpNodeID(hasEventSourceNode,obj.references[i].reftype)){
            continue;
        }

        obj.references[i].title = obj.references[i].displayname.text;
        nodes[0].addChildren(obj.references[i],"")
    }
    nodes[0].setExpanded( !expanded )
    nodes[0].setFocus();
    buildRefTable()
    buildContentTable()
}



function buildRefTable(){
    let node = opcuaTree.getFocusNode()
    const tbody = $("#refbody");
    const template = $("#refrow");
    tbody.empty();
    for(let i = 0 ; i < node.data.references.length ; i++){
        let clone = template[0].content.cloneNode(true);
        let th = clone.querySelectorAll("th");
        th[0].textContent = i;
        let td = clone.querySelectorAll("td");
        td[0].textContent = getNodeIDDisplay(node.data.references[i].reftype)
        td[1].textContent = node.data.references[i].displayname.text;
        tbody.append(clone);
    }
}

function buildContentTable(){
    opcuaContent.clear()
    rip( opcuaContent , obj);
    opcuaContent.expandAll()
}
