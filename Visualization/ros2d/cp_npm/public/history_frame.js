
var data;
var time;
var sig_name;
var viewtime;
var PrintHistory="없습니다";

const today = new Date();
today.setTime(0);

var hidx=0;
const FRAMES_PER_SECOND = 10;  // Valid values are 60,30,20,15,10...
const FRAME_MIN_TIME = (1000 / 60) * (60 / FRAMES_PER_SECOND) - (1000 / 60) * 0.5;
var lastFrameTime = 0;  // the last frame time


Kakao.init("b886eede39b9d47bc9d3cb6e91483799");   // 사용할 앱의 JavaScript 키를 설정

function shareKakaotalk(sig_name)
{
    Kakao.API.request({
      url: '/v2/api/talk/memo/default/send',
      data: {
        template_object: {
          object_type: 'text',
          text: sig_name+"가 발생한 것 같아요!",
          link: {
              web_url: 'http://192.168.0.193',
              mobile_web_url: 'http://192.168.0.193',
            },
            button_title : "BADA에서 확인하기"
        },
      },
      success: function(response) {
        console.log(response);
      },
      fail: function(error) {
        console.log(error);
      },
    });
  }
  function shareauthKakaotalk()
{
    Kakao.API.request({
      url: '/v2/api/talk/memo/default/send',
      data: {
        template_object: {
          object_type: 'text',
          text: "BADA 인증이 완료되었습니다",
          link: {
              web_url: 'http://192.168.0.193',
              mobile_web_url: 'http://192.168.0.193',
            },
            button_title : "BADA에서 확인하기"

        },
      },
      success: function(response) {
        console.log(response);
      },
      fail: function(error) {
        console.log(error);
      },
    });
  }
  shareauthKakaotalk();

  function printNow() {
    const today = new Date();
  
    // getDay: 해당 요일(0 ~ 6)를 나타내는 정수를 반환한다.
  
  
    const date = today.getDate();
    let hour = today.getHours();
    let minute = today.getMinutes();
    const ampm = hour >= 12 ? 'PM' : 'AM';
  
    // 12시간제로 변경
    hour %= 12;
    hour = hour || 12; // 0 => 12
  
    // 10미만인 분과 초를 2자리로 변경
    minute = minute < 10 ? '0' + minute : minute;

  
    var now = `     ${date}일   ${ampm} ${hour}:${minute}`;
    return now;
  };


function Queue(){

    this.dataStore = [];
    this.enqueue = enqueue;
    this.dequeue = dequeue;
    this.search=search;
    this.stoString=stoString;
}

function enqueue(element)
{
    this.dataStore.push(element);
}

function dequeue()
{   
    return this.dataStore.shift();
}


var cnt=0;
function search(){
    
    for(var i=0; i<this.dataStore.length;i++)
    {
      if((this.dataStore[i]-time)>1800000) //30분 이상이면 반복문탈출
      {
        break;
      }
      else{
        cnt++;
        console.log("Water events occur" + cnt + "times");
      }
    }
    if(cnt>=3)
    { console.log("Water event exceed 3 times. Send Message");
      return true;
    }
    else{
      return false;
    }
  }
  
  function stoString() {
    var retStr = "";
    for (var i = this.dataStore.length-1;i >=0; i-- )    {
        retStr +="  "+ this.dataStore[i]+"\n";
    }
    retStr = retStr.replace(/(?:\r\n|\r|\n)/g, '<br />');
    return retStr;
}



var water= new Queue();
var h_element;
var h = new Queue();



  var ros = new ROSLIB.Ros();

  // If there is an error on the backend, an 'error' emit will be emitted.
  ros.on('error', function (error) {
    console.log(error);
  });

  // Find out exactly when we made a connection.
  ros.on('connection', function () {
    console.log('Connection made!');
  });

  ros.on('close', function () {
    console.log('Connection closed.');
  });

  // Create a connection to the rosbridge WebSocket server.
  ros.connect('ws://localhost:9090');

  // Like when publishing a topic, we first create a Topic object with details of the topic's name
  // and message type. Note that we can call publish or subscribe on the same topic object.
  var listener = new ROSLIB.Topic({
    ros: ros,
    name: '/listener',
    messageType: 'std_msgs/String'
  });

  // Then we add a callback to be called every time a message is published on this topic.
  listener.subscribe(function (message) {
    // console.log('Received message on ' + listener.name + ': ' + message.data);

    // If desired, we can unsubscribe from the topic as well.
    listener.unsubscribe();
  });

  

  
  var hsignal = new ROSLIB.Topic({
    ros : ros,
    name : '/signal',
    //name : '/bada_audio/signal',
    messageType : 'std_msgs/String'
  });
  const dic1={'Cry':'아기 우는 소리', 'Alarm':'화재 경보', 'Door':'노크', 'Boiling':'물 끓는 소리', 'Silence':'조용해요', 'Water':'물소리', 'Bell':'초인종 소리'};

  hsignal.subscribe(function(m){

    sig_name=dic1[m.data];
    console.log("NOW SIGNAL : "+sig_name);


    time=today.getTime();
    viewtime=printNow();

    if(h.dataStore.length>=17)
    {
      h.dequeue();
    }
    
    if(sig_name==dic1['Water'])
    {
      //먼저 검색해 
      if(water.search())
      { 
        //총 3번 이상 발생했다면
        for(var i=1; i<=3; i++)
        {
          water.dequeue();
        }
        cnt=0;
        shareKakaotalk(sig_name);
        h.enqueue([sig_name, viewtime]);
        hidx=hidx+1;     
        PrintHistory=h.stoString();
      }
      else
      {
       //발생한적없다면
       water.enqueue(time);
      }
    }
    else if(sig_name!=dic1["Silence"])
    {
        shareKakaotalk(sig_name);
        h.enqueue([sig_name, viewtime]);
        hidx=hidx+1;       
        PrintHistory=h.stoString();
    }
    document.getElementById("History").innerHTML=PrintHistory;

  });




