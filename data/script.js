

  var ws = null;

document.addEventListener("DOMContentLoaded", function() {
  'use strict';



  function start(){

    ws = new WebSocket('ws://'+location.hostname+':2655/', ['arduino']);
    ws.onopen = function(){
      console.log('connected!');
    };
    ws.onmessage = function(e){
      console.log(e.data);
    };
    ws.onclose = function(){
      console.log('closed!');
      //reconnect now
      check();
    };

  }

  function check(){
    if(!ws || ws.readyState == 3) start();
  }

  start();

  setInterval(check, 5000);


});

function toggleDoor(){
  ws.send("toggledoor|");
  console.log('toggledoor');

}
