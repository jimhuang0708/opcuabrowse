const UA_NODEIDTYPE_NUMERIC    = 0;
const UA_NODEIDTYPE_STRING     = 3;
const UA_NODEIDTYPE_GUID       = 4;
const UA_NODEIDTYPE_BYTESTRING = 5;
const UA_NS0ID_ROOTFOLDER = 84;

/* ReferenceType */
const UA_NS0ID_ORGANIZES = 35
const UA_NS0ID_HASEVENTSOURCE = 36
const UA_NS0ID_HASMODELLINGRULE = 37
const UA_NS0ID_HASTYPEDEFINITION = 40
const UA_NS0ID_HASSUBTYPE = 45
const UA_NS0ID_HASPROPERTY = 46
const UA_NS0ID_HASCOMPONENT = 47
const UA_NS0ID_HASNOTIFIER = 48


const UA_ATTRIBUTEID_NODEID                  = 1;
const UA_ATTRIBUTEID_NODECLASS               = 2;
const UA_ATTRIBUTEID_BROWSENAME              = 3;
const UA_ATTRIBUTEID_DISPLAYNAME             = 4;
const UA_ATTRIBUTEID_DESCRIPTION             = 5;
const UA_ATTRIBUTEID_WRITEMASK               = 6;
const UA_ATTRIBUTEID_USERWRITEMASK           = 7;
const UA_ATTRIBUTEID_ISABSTRACT              = 8;
const UA_ATTRIBUTEID_SYMMETRIC               = 9;
const UA_ATTRIBUTEID_INVERSENAME             = 10;
const UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11;
const UA_ATTRIBUTEID_EVENTNOTIFIER           = 12;
const UA_ATTRIBUTEID_VALUE                   = 13;
const UA_ATTRIBUTEID_DATATYPE                = 14;
const UA_ATTRIBUTEID_VALUERANK               = 15;
const UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16;
const UA_ATTRIBUTEID_ACCESSLEVEL             = 17;
const UA_ATTRIBUTEID_USERACCESSLEVEL         = 18;
const UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19;
const UA_ATTRIBUTEID_HISTORIZING             = 20;
const UA_ATTRIBUTEID_EXECUTABLE              = 21;
const UA_ATTRIBUTEID_USEREXECUTABLE          = 22;
const UA_ATTRIBUTEID_DATATYPEDEFINITION      = 23;
const UA_ATTRIBUTEID_ROLEPERMISSIONS         = 24;
const UA_ATTRIBUTEID_USERROLEPERMISSIONS     = 25;
const UA_ATTRIBUTEID_ACCESSRESTRICTIONS      = 26;
const UA_ATTRIBUTEID_ACCESSLEVELEX           = 27;

const UA_NODECLASS_OBJECT = 1;
const UA_NODECLASS_VARIABLE = 2;
const UA_NODECLASS_METHOD = 4;
const UA_NODECLASS_OBJECTTYPE = 8;
const UA_NODECLASS_VARIABLETYPE = 16;
const UA_NODECLASS_REFERENCETYPE = 32;
const UA_NODECLASS_DATATYPE = 64;
const UA_NODECLASS_VIEW = 128;

function showIdRawString(id){
    return "ns:" + id.namespaceindex + " type:" + id.identifiertype + " identifier:" + id.identifier;
}

function getNodeClassDisplay(id){
    switch(id){
        case UA_NODECLASS_OBJECT:
            return "OBJECT"
        case UA_NODECLASS_VARIABLE:
            return "VARIABLE"
        case UA_NODECLASS_METHOD:
            return "METHOD"
        case UA_NODECLASS_OBJECTTYPE:
            return "OBJECTTYPE"
        case UA_NODECLASS_VARIABLETYPE:
            return "VARIABLETYPE"
        case UA_NODECLASS_REFERENCETYPE:
            return "REFERENCETYPE"
        case UA_NODECLASS_DATATYPE:
            return "DATATYPE"
        case UA_NODECLASS_VIEW:
            return "VIEW"
        default:
            return id
    }

}

function getNodeIDDisplay(id){
    if(id.namespaceindex == 0){
        if(id.identifiertype == UA_NODEIDTYPE_NUMERIC){
            switch(id.identifier){
                case UA_NS0ID_ORGANIZES:
                    return "ORGANIZES"
                case UA_NS0ID_HASEVENTSOURCE:
                    return "HASEVENTSOURCE"
                case UA_NS0ID_HASMODELLINGRULE:
                    return "HASMODELLINGRULE"
                case UA_NS0ID_HASTYPEDEFINITION:
                    return "HASTYPEDEFINITION"
                case UA_NS0ID_HASSUBTYPE:
                    return "HASSUBTYPE"
                case UA_NS0ID_HASPROPERTY:
                    return "HASPROPERTY"
                case UA_NS0ID_HASCOMPONENT:
                    return "HASCOMPONENT"
                case UA_NS0ID_HASNOTIFIER:
                    return "HASNOTIFIER"
                default:
                    return showIdRawString(id)
            }
        }
    }
    return showIdRawString(id)
}
