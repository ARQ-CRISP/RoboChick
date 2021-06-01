String MAIN_pageS = R"=====(
  <!DOCTYPE html>
  <html>
  <body bgcolor="ff0000">
  <center>
  <h1>Sound chick control system</h1><br>
)=====";

String MAIN_pageE = R"=====(
  Click to <a href="experiment">START EXPERIMENT</a><br>
  Click to <a href="viewConfig">SEE EXPERIMENT SETUP</a><br>
  Click to <a href="sound1">PLAY CALLS AT 4s SILENT INTERVALS</a><br>
  Click to <a href="sound2">PLAY CALLS AT 5s SILENT INTERVALS</a><br>
  Click to <a href="sound3">PLAY CALLS AT 6s SILENT INTERVALS</a><br>
  Click to <a href="vibrate">VIBRATE</a><br>
  Click to <a href="expStop">END EXPERIMENT</a><br>
  Click to <a href="calibrate">CALIBRATE</a><br>
  Click to <a href="results">SEE RESULTS</a><br>
  
  <hr>
  <a href="https://www.qmul.ac.uk/robotics/">ARQ - Advanced Robotics at QMUL</a>
  </center>

  </body>
  </html>
)=====";

String EXPERIMENT_page = R"=====(
  <!DOCTYPE html>
  <html>
  <body bgcolor="00ff00">
  <center>
  <h1>Experiment setup</h1><br>
  <form action="/config" method="POST">
    Arena:
    <input type="text" name="arena" size = "2" value="00">
    <br><br>
    Chick:
    <input type="text" name="chickID" size = "3" value="000">
    <br><br>
    Date:
    <input type="text" name="date" size = "6" value="DDMMYY">
    <br><br>
    Experiment length:  
    <input type="text" name="lengthHR" size="2" value="1"> hr  :  <input type="text" name="lengthMIN" size="2" value="0"> min
    <br><br>
    Detection threshold:
    <input type="text" name="sensitivity" size = "3" value="0">
    <br>(lower is more sensitive, 0 is a continuous vibration)<br>
    Continuous vibration pause length: <input type="text" name="pauseLen" size="3" value="0">ms <br>
    <input type="checkbox" name="flashing" value="F"> Flashing <br>
    <br><br>
    Response on peck:<br>
    <input type="checkbox" name="responseVibrate" value="V"> Vibrate for <input type="text" name="vibrateLength" size="4" value="300">ms with <input type="text" name="vibrateIntensity" size="3" value="66">% intensity<br>
    <input type="checkbox" name="responseS1" value="S1"> Sound 1 <br>
    <input type="checkbox" name="responseS2" value="S2"> Sound 2 <br>
    <input type="checkbox" name="light" value="L"> Light <br>
    <input type="checkbox" name="approachSound" value="A1"> Sound when approaching? <br>

    <br>
    <input type="submit" value="Start">
  </form>
  <br>Click to <a href="results">SEE RESULTS</a>
  </center>

  </body>
  </html>
)=====";

String SUCCESS_page = R"=====(
  <!DOCTYPE html>
  <html>
  <body bgcolor="ff0000">
  <center>
  <h1>Starting experiment...</h1><br>
  <a href="results">RESULTS</a><br>
  <hr>
  <a href="https://www.qmul.ac.uk/robotics/">ARQ - Advanced Robotics at QMUL</a>
  </center>

  </body>
  </html>
)=====";

String RESULTS_pageS = R"=====(
  <!DOCTYPE html>
  <html>
  <body bgcolor="ff0000">
  <center>
  <h1>Experiment results</h1><br>
  <a href="../">HOME</a><br>
  <a href="results">REFRESH</a><br><hr>
)=====";

String CONFIG_pageS = R"=====(
  <!DOCTYPE html>
  <html>
  <body bgcolor="ff0000">
  <center>
  <h1>Experiment Settings</h1><br>
  <a href="../">HOME</a><br><hr>
)=====";
  

String RESULTS_pageE = R"=====(
  <hr>
  <a href="https://www.qmul.ac.uk/robotics/">ARQ - Advanced Robotics at QMUL</a>
  </center>
  </body>
  </html>
)=====";
