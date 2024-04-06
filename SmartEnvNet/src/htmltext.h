char page1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 AC Fan & LDR Sensor Control</title>
  <style>
    body {font-family: Arial; text-align: center;}
    h1 {margin-top: 20px;}
    p {font-size: 20px; margin-top: 10px; margin-bottom: 5px;}
    button {font-size: 20px; padding: 10px; margin-top: 10px;}
    .start {background-color: green; color: white;}
    .stop {background-color: red; color: white;}
    .records {background-color: blue; color: white;}
    .sensor-data {margin-top: 20px;}
    .record-table {width: 100%;border-collapse: collapse;}
    .record-table th, .record-table td {padding: 8px;border: 1px solid #ddd;text-align: center;}
    .record-table th {background-color: #f2f2f2;}
    .record-table tr:nth-child(even) {background-color: #f2f2f2;}
    .record-table tr:hover {background-color: #ddd;}
  </style>
</head>
<body>
  <h1>ESP32 AC Fan & LDR Sensor Control</h1>
  <p>Use the links below to navigate:</p>
  <ul>
      <li><a href="/temperature">Temperature Page</a></li>
      <li><a href="/config">Configuration Page</a></li>
  </ul>
  <h1>Sensor Readings</h1>
  <p id="temperature">Current Temperature: --°C</p>
  <p id="humidity">Current Humidity: --%</p>
  <p id="lightIntensity">Current Light Intensity: --</p>
  <br>
  <button class='start' onclick='startFan()'>Start AC Fan</button>
  <button class='stop' onclick='stopFan()'>Stop AC Fan</button>
  <button class='records' onclick='showLDRRecords()'>Show Last 25 LDR Records</button>
  
  <div id='ldr_records' class='sensor-data'></div>

  <script>
    function startFan() {
      var xhttp = new XMLHttpRequest();
      xhttp.open('GET', '/startFan', true);
      xhttp.send();
    }

    function stopFan() {
      var xhttp = new XMLHttpRequest();
      xhttp.open('GET', '/stopFan', true);
      xhttp.send();
    }

    function showLDRRecords() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var records = JSON.parse(this.responseText);
          var html = '<table class="record-table"><tr><th>Timestamp</th><th>LDR Value</th></tr>';
          records.forEach(function(record) {
            html += '<tr><td>' + record.Timestamp + '</td><td>' + record.value + '</td></tr>';
          });
          html += '</table>';
          document.getElementById('ldr_records').innerHTML = html;
        }
      };
      xhttp.open('GET', '/getLDRRecords', true);
      xhttp.send();
    }

  function fetchSensorValue() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          document.getElementById('temperature').innerHTML = "Current Temperature: " + data.temperature + "°C";
          document.getElementById('humidity').innerHTML = "Current Humidity: " + data.humidity + "%";
          document.getElementById('lightIntensity').innerHTML = "Current Light Intensity: " + data.lightIntensity;
        }
      };
      xhttp.open('GET', '/sensorValue', true);
      xhttp.send();
    }

    fetchSensorValue();
    setInterval(fetchSensorValue, 5000);
  </script>
</body>
</html>
)rawliteral";


const char page2[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Temperature & Humidity Data</title>
    <script>
      function fetchData() {
        fetch('/data')
          .then(response => response.json()) // Process the response as JSON
          .then(data => {
            document.getElementById('temperature').textContent = "Temperature: " + data.temperature + "°C";
            document.getElementById('humidity').textContent = "Humidity: " + data.humidity + "%";
          })
          .catch(error => console.error('Error:', error));
      }

      document.addEventListener('DOMContentLoaded', function() {
        fetchData();
        setInterval(fetchData, 5000);
      });
    </script>
</head>
<body>
    <h1>Temperature & Humidity Data</h1>
    <div id="dataContainer">Loading data...</div>
</body>
</html>
)rawliteral";


const char page3[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Device Configuration</title>
</head>
<body>
    <h1>Device Configuration</h1>
    <form id="configForm">
        <label for="deviceId">Device ID/Name:</label><br>
        <input type="text" id="deviceId" name="deviceId"><br>
        
        <label for="commMethod">Communication Method:</label><br>
        <select id="commMethod" name="commMethod">
            <option value="http">HTTP POST</option>
            <option value="mqtt">MQTT Publish</option>
        </select><br>
        
        <label for="manualOverride">Manual Override:</label><br>
        <input type="checkbox" id="manualOverride" name="manualOverride"><br>
        
        <label for="triggerTemp">Trigger Temperature:</label><br>
        <input type="number" id="triggerTemp" name="triggerTemp" step="0.1"><br>
        
        <input type="button" value="Submit" onclick="submitConfig()">
    </form>
    
    <script>
      function submitConfig() {
        const form = document.getElementById('configForm');
        const formData = new FormData(form);
        const data = {};
        formData.forEach((value, key) => {data[key] = value;});
        fetch('/updateConfig', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify(data),
        })
        .then(response => response.text())
        .then(data => {
          alert('Configuration updated successfully!');
        })
        .catch((error) => {
          console.error('Error:', error);
        });
      }
    </script>
</body>
</html>
)rawliteral";