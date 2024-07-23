local serviceId

function OnInit(id)
    print("[lua] ping OnInit id:"..id)
    serviceId = id
end

function OnServiceMsg(source, buff)
    print("[lua] ping OnServiceMsg id:"..serviceId)

    if string.len(buff) > 50 then
        sunnet.KillService(serviceId)
        return
    end

    sunnet.Send(serviceId, source, buff.."i")
end

function OnExit()
    print("[lua] ping OnExit")
end