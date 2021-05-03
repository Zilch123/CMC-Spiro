/*------------------------------------------------------------------------------
  11/09/2020
  Author: Timoth Dev
  Platforms: ESP8266
  Language: C++/Arduino + HTML/JS
  File: ESP_plot.ino
  ------------------------------------------------------------------------------
  Description:
  Input: HX711 chip is connected to a pressure bridge.
  The Hx711 amplifies the diffrential input and converts it into a 24 bit ADC.
  The 24 bit output is fed to ESP8266 which streams it to a web-broswer.
  Output: HTML JS webpage to plot and download streamed data.
  ------------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266TimerInterrupt.h>
#include <Wire.h>
#include "HX711_.h"
//#define HW_TIMER_INTERVAL_MS        50

//Using Timer 1
//ESP8266Timer Timer1;

// Connecting to the Internet
char * ssid = "TabulaRasa";
char * password = "t135792468";
long xn;
long prev_data;
long abs_prev_data;
long abs_xn;
const int startBtn = 4;
int buttonState = 0;

// Running a web server
ESP8266WebServer server;

//Initiallising HX711 as cell
HX711 cell(D2, D3);

// Adding a websocket to the server
WebSocketsServer webSocket = WebSocketsServer(81);

// Serving a web page (from flash memory)
// formatted as a string literal
char webpage[] PROGMEM = R"=====(
<html>
<!-- Adding a data chart using Chart.js -->
<head>
  <script src='https://cdn.plot.ly/plotly-latest.min.js'></script>
</head>
<body onload="javascript:init()">
<tr>
    <td><input type="button" id="mixBut" value="Start" /></td>
    <td><input type="button" id="clearBtn" onclick="clear_()" value="Clear"/></td>
    <td>Filename:</td>
    <td><input id="INPUT"></input></td>
    <td><button onclick="saveTextAsFile()">Download</button></td>
    <td><button onclick="baseline()">Baseline</button></td>
    <td><button onclick="maxInspiration()">Maximal Inspiration</button></td>
    <td><button onclick="maxExpiration()">Maximal Expiration</button></td>
    <p id="divMsg"></p>
</tr>
<hr />
<div id="graph" style="width:100%;height:80%;"></div>
<!-- Adding a websocket to the client (webpage) -->
<script>
  var t = 0;
  var t_=0.0;
  var t_insp_start = 0;
  var df = [];
  var df_filt = [];
  var df_t = [];
  var df1 = [];
  var df_t1 = [];
  var auc_df1 = [];
  var df2 = [];
  var df_t2 = [];
  var auc_df2 = [];
  var df3 = [];
  var df_t3 = [];
  var auc_df3 = [];
  var df4 = [];
  var df_t4 = [];
  var auc_df4 = [];
  var selected_data_x = [];
  var selected_data_y = [];
  var baseline_x =[];
  var baseline_y =[];
  var AUC_array =[];
  var auc_value = 0;
  var avgBaseline = 0;
  var previousTime = 0;
  var previousValue = 0;
  var tempAUC= 0;
  var auc_ = 0;
  var auc_df = [];
  var inspT=[];
  var range_ = 20000;

  const arrAvg = arr => arr.reduce((a,b) => a + b, 0) / arr.length;
  var mixBut = document.getElementById("mixBut");
  mixBut.addEventListener("click", Start);
  var webSocket, dataPlot;
  var recordFlag = 0;
  var startTime = new Date();
  var data_empty0 = {
    x: [],
    y: [],
    mode: 'lines',
    name: 'Flow Rate',
    scatter: {color: '#80CAF6'}
    };
  var data_empty1 = {
    x: [],
    y: [],
    yaxis: 'y2',
    mode: 'lines',
    name: 'AUC',
    scatter: {color: '#FF0000'}
    };
    
  var data_empty = [data_empty0, data_empty1];
  
  var layout = {
    dragmode: 'select',
    yaxis: {range: [-range_, range_]}, 
    yaxis2: {range: [-range_, range_], overlaying: 'y'}
    };
  Plotly.plot('graph', data_empty, layout);  
  
  function init() {
    webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
    webSocket.onmessage = function(event) {
      if(recordFlag===1)
      {
        var data = JSON.parse(event.data);
        var now = new Date();
        t = Math.abs(startTime - now)/1000;
//        t += 0.02;
        t_ = round_(t,2);
        if(t_<=30)
        {
          df_t.push(t_);
          df.push(data.value);
        }
        else if(t_>30 && t_<=60)
        {
          df_t1.push(t_);
          df1.push(data.value);          
        }
        else if(t_>60 && t_<=90)
        {
          df_t2.push(t_);
          df2.push(data.value);
        }
        else if(t_>90 && t_<=120)
        {
          df_t3.push(t_);
          df3.push(data.value);
        }
        else if(t_>120 && t_<=150)
        {
          df_t4.push(t_);
          df4.push(data.value);
        }
        console.log(data.value);
        if (avgBaseline ===0)
        {auc_df.push(0);}
        else
        {
          d1 = previousValue - avgBaseline;
          d2 = data.value - avgBaseline;
          auc_ += 0.5*(d1+d2)*0.02;   // 0.5 
          auc_ += data.value;
          if(Math.sign(d1)=== -1 && Math.sign(d2)===1)
          {
            auc_=0;
          }
          if(Math.sign(d1)=== 1 && Math.sign(d2)===-1)
          {
            auc_=0;
          }
          
        if(t_<=30)
        {
          auc_df.push(auc_);
        }
        else if(t_>30 && t_<=60)
        {
          auc_df1.push(auc_);         
        }
        else if(t_>60 && t_<=90)
        {
          auc_df2.push(auc_);
        }
        else if(t_>90 && t_<=120)
        {
          auc_df3.push(auc_);
        }
        else if(t_>120 && t_<=150)
        {
          auc_df4.push(auc_);
        }
      }
        
          previousValue = data.value.valueOf();
      }
    }
  }
  function baseline(){
    if(recordFlag ===0){
      df =[];
      df_t =[];
      auc_df=[];
      recordFlag = 1;
    }
    else{
      alert("Please stop recording and restart from baseline");
    }
  }
  function maxInspiration(){
    if(recordFlag ===0){
      alert("Please start baseline");
    }
      avgBaseline = arrAvg(df);
      document.getElementById("divMsg").innerHTML = "Baseline: " + round_(avgBaseline,2);

      Plotly.extendTraces('graph', {x:  [df_t],
                          y: [df]}, [0]);
      if(df_t1.length >0)
      {
        Plotly.extendTraces('graph', {x:  [df_t1],
                          y: [df1]}, [0]);
      }
      if(df_t2.length >0)
      {
        Plotly.extendTraces('graph', {x:  [df_t2],
                          y: [df2]}, [0]);
      }
      if(df_t3.length >0)
      {
        Plotly.extendTraces('graph', {x:  [df_t3],
                          y: [df3]}, [0]);
      }
      if(df_t4.length >0)
      {
        Plotly.extendTraces('graph', {x:  [df_t4],
                          y: [df4]}, [0]);
      }
      t_insp_start = t.valueOf();
    }
  function maxExpiration(){
    if(recordFlag ===0){
      alert("Please start from baseline");
    }
    else{
      if(avgBaseline===0){
        alert("No Baselinefound");
      }
      else{
        recordFlag = 0;
        Array.prototype.push.apply(df_t,df_t1);
        Array.prototype.push.apply(df_t,df_t2);
        Array.prototype.push.apply(df_t,df_t3);
        Array.prototype.push.apply(df_t,df_t4);
        Array.prototype.push.apply(df,df1);
        Array.prototype.push.apply(df,df2);
        Array.prototype.push.apply(df,df3);
        Array.prototype.push.apply(df,df4);
        Array.prototype.push.apply(auc_df,auc_df1);
        Array.prototype.push.apply(auc_df,auc_df2);
        Array.prototype.push.apply(auc_df,auc_df3);
        Array.prototype.push.apply(auc_df,auc_df4);
        
        inspAUCVolume = AUC(df_t,df,avgBaseline);
        inspAUC = inspAUCVolume[0];
        peakInspVolume = inspAUCVolume[1];
        var df_t_start = df_t.indexOf(t_insp_start);
        inspT = df_t.slice(df_t_start,df_t.length);
        var df_insT = df.slice(df_t_start,df_t.length);
        Plotly.extendTraces('graph', {x:  [df_t,df_t],y: [df,auc_df]}, [0,1]);
        console.log(auc_df.length,inspT.length);
        document.getElementById("divMsg").innerHTML = "Baseline:  " + round_(avgBaseline,2)
                                                     +"       Inspiration AUC: " + round_(peakInspVolume,2)
                                                     +"       DT: "+ round_((Math.max(...df_t)-Math.min(...df_t)),2)+" s" 
                                                     +"       Max Inspiratory Volume: " + round_(Math.max(...inspAUC),2);

      }
    }
  }
  function AUC(xdata,ydata,baselinedata){
    auc_value = 0;
    if(xdata.length >0){
        time_auc = [...xdata];
        baseline_corrected_y = ydata.map( function(value) {
        return value - baselinedata; } );
        for(i=0; i < (time_auc.length-1);i++){
          tempAUC = 0.5*(baseline_corrected_y[i]+baseline_corrected_y[i+1])*(time_auc[i+1]-time_auc[i]);
          AUC_array.push(tempAUC);
          auc_value += tempAUC; 
        }
        return [AUC_array, auc_value]; 
    }else{
      alert("No baseline or data found");
    }
  }
   window.onkeydown = function(e){
    if ( e.target.nodeName == 'INPUT' ) return;
    handle_shortcut();
  };
  
  function handle_shortcut(){
    if (event.key === "s") 
    {
      if(recordFlag === 0){
        recordFlag = 1;
      }else{
        recordFlag = 0;
      }
    }
    if (event.key === "b") 
    {
        baseline();
    }
    if (event.key === "i") 
    {
        maxInspiration();
    }
    if( event.key ==="e"){
      maxExpiration();
    }
    if( event.key ==="d"){
      saveTextAsFile();
    }
    if( event.key ==="c"){
      clear_();
    }
  }
  function round_(num, places)
  { return +(Math.round(num + "e+" + places)  + "e-" + places);
  }
  function saveTextAsFile(){
    var recording2Save = [];
    console.log(df.length, df_t.length);
    for (var i=0; i<df.length; i++){
//     recording2Save[i] = [df_t[i],"\t", df[i], "\r\n"]; 
//    } 
     recording2Save[i] = [round_(df_t[i],0)]
    }
    result = { };
    for(var i = 0; i < recording2Save.length; ++i) {
        if(!result[recording2Save[i]])
            result[recording2Save[i]] = 0;
        ++result[recording2Save[i]];
    }
    var textToSave = "Time\tUC_Flow\r\n"+ JSON.stringify(result, null, 4).toString().split(",").join("");
    
//    var textToSave = "Time\tUC_Flow\r\n"+ recording2Save.toString().split(",").join("");
    var textToSaveAsBlob = new Blob([textToSave], {type:"text/plain"});
    var textToSaveAsURL = window.URL.createObjectURL(textToSaveAsBlob);
    var fileNameToSaveAs = document.getElementById("INPUT").value;

    var downloadLink = document.createElement("a"); 
    downloadLink.download = fileNameToSaveAs;
    downloadLink.innerHTML =  "Download File";
    downloadLink.href = textToSaveAsURL;
    downloadLink.onclick = destroyClickedElement;
    downloadLink.style.display = "none";
    document.body.appendChild(downloadLink);
    downloadLink.click();
    
    df.splice(0,df.length);
    df_t.splice(0,df_t.length);
}

function Start(){
    console.log("Started");
    mixBut.removeEventListener("click", Start);
    mixBut.addEventListener("click", Stop);
    mixBut.value = "Stop";
    recordFlag = 1;
}

function Stop(){
    console.log("Stopped");
    mixBut.removeEventListener("click", Stop);
    mixBut.addEventListener("click", Start);
    mixBut.value = "Start";
    recordFlag = 0;
    var startTime = new Date(); 
  }

function clear_()
{
   location.reload(false); 
}
var graphDiv = document.getElementById('graph');
var color1Light = '#7b3294';
graphDiv.on('plotly_selected', function(eventData) {
  var colors = [];
  eventData.points.forEach(function(pt) {
    selected_data_x.push(pt.x);
    selected_data_y.push(pt.y);
  });
  var update1 = {
                x:  [[selected_data_x]],
                y: [[selected_data_y]]
               };
  Plotly.restyle(graphDiv, 'marker.color', color1Light, [0]);
  });

function destroyClickedElement(event)
{
    document.body.removeChild(event.target);
}
</script>
</body>
</html>
)=====";

void setup() {
//  pinMode(startBtn, INPUT);
  WiFi.begin(ssid, password);
   Serial.begin(115200);
   Serial.print("start");
  while(WiFi.waitForConnectResult()!= WL_CONNECTED){
     Serial.print(WiFi.status());
     delay(500);
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",[](){
  server.send_P(200, "text/html", webpage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.print("connected");
  prev_data = cell.get_value();
}

void loop() {
  webSocket.loop();
  server.handleClient();
  
//  buttonState = digitalRead(startBtn);
  xn = cell.read(); 
//  abs_xn = abs(xn);
//  abs_prev_data = abs(prev_data);
//  if(abs_xn-abs_prev_data > 600000 || abs_xn-abs_prev_data < -600000){
//    xn = prev_data;
//  }
  String json = "{\"value\":";
  json += xn;
  json += "}";
  webSocket.broadcastTXT(json.c_str(), json.length());
  delay(10);
//  prev_data = xn;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  // Do something with the data from the client
  if(type == WStype_TEXT){
    float dataRate = (float) atof((const char *) &payload[0]);
  }
}
