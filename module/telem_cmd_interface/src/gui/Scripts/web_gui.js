//struct for HDTN Data Rate graph
var rate_data_ingress = [{
    x: [],
    y: [],
    name: 'ingress',
    type: 'scatter',
    line: {
        color: "aqua"
    }
}];

var rate_data_egress = [{
    x: [],
    y: [],
    name: 'egress',
    type: 'scatter',
    line: {
        color: "magenta"
    }
}];

//Style for HDTN Data Rate Graph
var ingressLayout = {
    title: 'Ingress Data Rate',
    paper_bgcolor: "#404040",
    plot_bgcolor: "#404040",
    width: 700,
    height: 450,
    xaxis: {
        title: "Timestamp (s)",
    },
    yaxis: {
        title: "Data Rate (Mbps)",
    },
    font:{
        family: "Arial",
        size: 18,
        color: "white"
    }
};

var egressLayout = {
    title: 'Egress Data Rate',
    paper_bgcolor: "#404040",
    plot_bgcolor: "#404040",
    width: 700,
    height: 450,
    xaxis: {
        title: "Timestamp (s)",
    },
    yaxis: {
        title: "Data Rate (Mbps)",
    },
    font:{
        family: "Arial",
        size: 18,
        color: "white"
    }
};

// Supported convergence layers
const StcpConvergenceLayer = 1;
const LtpConvergenceLayer = 2;

// Given a data view and a byte index, updates an HTML element with common outduct data
updateElementWithCommonOutductData = (htmlElement, dv, byteIndex) => {
    const totalBundlesAcked = dv.getUint64(byteIndex, littleEndian);
    htmlElement.querySelector("#totalBundlesAcked").innerHTML = totalBundlesAcked.toFixed();
    byteIndex += 8;
    const totalBytesAcked = dv.getUint64(byteIndex, littleEndian);
    htmlElement.querySelector("#totalBytesAcked").innerHTML = totalBytesAcked.toFixed();
    byteIndex += 8;
    const totalBundlesSent = dv.getUint64(byteIndex, littleEndian);
    htmlElement.querySelector("#totalBundlesSent").innerHTML = totalBundlesSent.toFixed();
    byteIndex += 8;
    const totalBytesSent = dv.getUint64(byteIndex, littleEndian);
    htmlElement.querySelector("#totalBytesSent").innerHTML = totalBytesSent.toFixed();
    byteIndex += 8;
    const totalBundlesFailedToSend = dv.getUint64(byteIndex, littleEndian);
    htmlElement.querySelector("#totalBundlesFailedToSend").innerHTML = totalBundlesFailedToSend.toFixed();
    byteIndex += 8;
    return byteIndex;
}

// Given a data view and a byte index, updates an HTML element with STCP specific data
updateStcpOutduct = (dv, byteIndex, outductPos) => {
    // Attempt to find an existing STCP card by the outduct position
    // If that fails, create a new one by cloning the template
    const uniqueId = "stcpCard" + outductPos;
    var card = document.getElementById(uniqueId);
    if (!card) {
        const template = document.getElementById("stcpTemplate");
        card = template.cloneNode(true);
        card.id = uniqueId;
        card.classList.remove("hidden");
        template.parentNode.append(card);
    }
    const displayName = "Outduct " + (outductPos +1).toFixed();
    card.querySelector("#cardName").innerHTML = displayName;

    byteIndex = updateElementWithCommonOutductData(card, dv, byteIndex)

    const totalStcpBytesSent = dv.getUint64(byteIndex, littleEndian);
    card.querySelector("#totalStcpBytesSent").innerHTML = totalStcpBytesSent.toFixed();
    byteIndex += 8;
    return byteIndex;
}

// Given a data view and a byte index, updates an HTML element with LTP specific data
updateLtpOutduct = (dv, byteIndex, outductPos) => {
    // Attempt to find an existing LTP card by the outduct position
    // If that fails, create a new one by cloning the template
    const uniqueId = "ltpCard" + outductPos;
    var card = document.getElementById(uniqueId);
    if (!card) {
        const template = document.getElementById("ltpTemplate");
        card = template.cloneNode(true);
        card.id = uniqueId;
        card.classList.remove("hidden");
        template.parentNode.append(card);
    }
    const displayName = "Outduct " + (outductPos + 1).toFixed();
    card.querySelector("#cardName").innerHTML = displayName;

    byteIndex = updateElementWithCommonOutductData(card, dv, byteIndex)

    const numCheckpointsExpired = dv.getUint64(byteIndex, littleEndian);
    card.querySelector("#numCheckpointsExpired").innerHTML = numCheckpointsExpired.toFixed();
    byteIndex += 8;
    const numDiscretionaryCheckpointsNotResent = dv.getUint64(byteIndex, littleEndian);
    card.querySelector("#numDiscretionaryCheckpointsNotResent").innerHTML = numDiscretionaryCheckpointsNotResent.toFixed();
    byteIndex += 8;
    const countUdpPacketsSent = dv.getUint64(byteIndex, littleEndian);
    card.querySelector("#countUdpPacketsSent").innerHTML = countUdpPacketsSent.toFixed();
    byteIndex += 8;
    const countRxUdpBufferOverruns = dv.getUint64(byteIndex, littleEndian);
    card.querySelector("#countRxUdpBufferOverruns").innerHTML = countRxUdpBufferOverruns.toFixed();
    byteIndex += 8;
    const countTxUdpPacketsLimitedByRate = dv.getUint64(byteIndex, littleEndian);
    card.querySelector("#countTxUdpPacketsLimitedByRate").innerHTML = countTxUdpPacketsLimitedByRate.toFixed();
    byteIndex += 8;

    return byteIndex;
}

//Launch Data Graphs
Plotly.newPlot("ingress_rate_graph", rate_data_ingress, ingressLayout, { displaylogo: false });
Plotly.newPlot("egress_rate_graph", rate_data_egress, egressLayout, { displaylogo: false });


//Struct for Pie Chart of bundle destinations
var pie_data = [{
    //add variables for storage: egress ratio
    values: [0,0],
    labels: ['Storage', 'Egress'], 
    type: 'pie'
}];

//Style for Pie Chart
var pie_layout = {
    title: 'Bundle Destinations',
    height: 450,
    width:450,
    paper_bgcolor: "#404040",
    font:{
        family: "Arial",
        size: 18,
        color: "white"
    }
};

//Launch Pie Chart
Plotly.newPlot('storage_egress_chart', pie_data, pie_layout, { displaylogo: false });

var ingressRateCalculator = new RateCalculator();
var egressRateCalculator = new RateCalculator();

window.addEventListener("load", function(event){
    if(!("WebSocket" in window)){
        alert("WebSocket is not supported by your Browser!");
    }

    var wsproto = (location.protocol === 'https:') ? 'wss:' : 'ws:';
    connection = new WebSocket(wsproto + '//' + window.location.host + '/websocket');
    connection.binaryType = "arraybuffer";

    //When connection is established
    connection.onopen = function(){
        console.log("ws opened");
    }

    //When connection is closed
    connection.onclose = function(){
        console.log("ws closed");
    }

    //When client receives data from server
    connection.onmessage = function(e){
        console.log("rcvd");
        //if binary data
        if(e.data instanceof ArrayBuffer){
            console.log("Binary Data Received");

            littleEndian = true;
            //https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView
            DataView.prototype.getUint64 = function(byteOffset, littleEndian) {
                // split 64-bit number into two 32-bit (4-byte) parts
                const left =  this.getUint32(byteOffset, littleEndian);
                const right = this.getUint32(byteOffset+4, littleEndian);

                // combine the two 32-bit values
                const combined = littleEndian? left + 2**32*right : 2**32*left + right;

                if (!Number.isSafeInteger(combined))
                    console.warn(combined, 'exceeds MAX_SAFE_INTEGER. Precision may be lost');

                return combined;
            }
    
            var dv = new DataView(e.data);
            var byteIndex = 0;
            var type = dv.getUint64(byteIndex, littleEndian);
            byteIndex += 8;
        
            if(type == 1){
                //Ingress
                var totalDataBytes = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                var bundleCountEgress = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                var bundleCountStorage = dv.getUint64(byteIndex, littleEndian);
                ingressRateCalculator.appendVal(totalDataBytes);
                if (ingressRateCalculator.count == 1) {
                    // Don't plot anything the first time through, since we don't have
                    // sufficient data
                    return;
                }

                rate_data_ingress[0]['x'].push(ingressRateCalculator.count);
                rate_data_ingress[0]['y'].push(ingressRateCalculator.currentMbpsRate);
                Plotly.update("ingress_rate_graph", rate_data_ingress, ingressLayout);
                pie_data[0]['values'] = [bundleCountStorage, bundleCountEgress];
                Plotly.update('storage_egress_chart', pie_data, pie_layout);
                document.getElementById("rate_data").innerHTML = ingressRateCalculator.currentMbpsRate.toFixed(3);
                document.getElementById("max_data").innerHTML = Math.max.apply(Math, rate_data_ingress[0].y).toFixed(3);
                document.getElementById("avg_data").innerHTML = ingressRateCalculator.averageMbpsRate.toFixed(3);
                document.getElementById("ingressBundleData").innerHTML = totalDataBytes.toFixed(2);
                document.getElementById("ingressBundleCountStorage").innerHTML = bundleCountStorage;
                document.getElementById("ingressBundleCountEgress").innerHTML = bundleCountEgress;
                document.getElementById("ingressBundleCount").innerHTML = bundleCountStorage + bundleCountEgress;

            }
            else if(type == 2){
                //Egress
                egressBundleCount = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                var egressTotalDataBytes = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                var egressMessageCount = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                egressRateCalculator.appendVal(egressTotalDataBytes);
                if (egressRateCalculator.count == 1) {
                    // Don't plot anything the first time through, since we don't have
                    // sufficient data
                    return;
                }

                rate_data_egress[0]['x'].push(egressRateCalculator.count);
                rate_data_egress[0]['y'].push(egressRateCalculator.currentMbpsRate);
                Plotly.update("egress_rate_graph", rate_data_egress, egressLayout);

                document.getElementById("egressDataRate").innerHTML = egressRateCalculator.currentMbpsRate.toFixed(3);
                document.getElementById("egressBundleCount").innerHTML = egressBundleCount;
                document.getElementById("egressBundleData").innerHTML = egressTotalDataBytes.toFixed(2);
                document.getElementById("egressMessageCount").innerHTML = egressMessageCount;

                // Handle any outduct data
                var index = 0;
                while (byteIndex < dv.byteLength) {
                    const convergenceLayerType = dv.getUint64(byteIndex, littleEndian);
                    byteIndex += 8;

                    if (convergenceLayerType == StcpConvergenceLayer) {
                        byteIndex = updateStcpOutduct(dv, byteIndex, index++);
                    }
                    if (convergenceLayerType == LtpConvergenceLayer) {
                        byteIndex = updateLtpOutduct(dv, byteIndex, index++);
                    }
                }
            }
            else if(type == 3){
                //Storage
                var totalBundlesErasedFromStorage = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                var totalBundlesSentToEgressFromStorage = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                var usedSpaceBytes = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                var freeSpaceBytes = dv.getUint64(byteIndex, littleEndian);
                byteIndex += 8;
                document.getElementById("totalBundlesErasedFromStorage").innerHTML = totalBundlesErasedFromStorage;
                document.getElementById("totalBundlesSentToEgressFromStorage").innerHTML = totalBundlesSentToEgressFromStorage;
                document.getElementById("usedSpaceBytes").innerHTML = usedSpaceBytes;
                document.getElementById("freeSpaceBytes").innerHTML = freeSpaceBytes;
            }
        }
        //else this is text data
        else{
            if(e.data === "Hello websocket"){
                console.log(e.data);
            }
            else{
                try{
                    console.log(e.data);
                    var obj = JSON.parse(e.data); //could error based on encodings
                    console.log("JSON data parsed");
                    UpdateData(obj);
                }
                catch(err){
                    console.log(err.message);
                }
            }
        }
    }
});
