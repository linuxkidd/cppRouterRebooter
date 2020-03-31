#define VERSION "2020-03-30 22:05"

const char* redirectIndex = 
"<!doctype html> <html lang=\"en\"> <head> <meta charset=\"utf-8\"> <title>RouterRebooter</title>"
"<meta http-equiv='Refresh' content='%u; url=/' /></head><body><h3>%s</h3><br />Redirecting to <a href='/'>main page</a> in %u seconds.</body></html>";
const char* serverIndex = 
"<!doctype html> <html lang=\"en\"> <head> <meta charset=\"utf-8\"> <title>RouterRebooter</title>"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"> <meta name=\"viewport\" content=\"width=device-width,user-scalable=no\"> <meta name=\"description\" content=\"RouterRebooter\"> <meta name=\"author\" content=\"linuxkidd\">"
"<script type='text/javascript'>"
"window.onload = fetchStatus; "
"function fetchStatus() { fetch('/status').then((response) => { return response.json() }).then((data) => { "
"['wifi_name','wifi_rssi','lastms','countFail','countSuccess','routerReset','epoch','ip','mqtt_server','mqtt_topic_base','test_server','test_port','test_interval','test_fail_reset','test_success_clear','relay_min_on','relay_off_time'].forEach(function(item) { if(item in data) { if(item==\"epoch\") { if(data[item]>100000) { var dt=new Date(); var d = new Date((parseFloat(data[item])-(dt.getTimezoneOffset()*60))*1000); document.getElementById(item).value=d.toISOString().replace(\"T\",\" \").slice(0,19);  } else { document.getElementById(item).value = 'booted for ' + String(data[item])+' seconds'; } } else document.getElementById(item).value = data[item]; }})}); setTimeout(fetchStatus,30000);}"
"</script>"
"<style>"
"table.state tr:nth-child(even) { background: #ddd; }"
"th { text-align: left; }"
"tr { text-align: right; }"
"input { text-align: right; }"
"input:disabled { color: black; background: transparent; border: 0px; }"
".custom-file-upload, a { background-color: red; box-shadow: 0 5px 0 darkred; color: white; padding: 1em 1.5em; margin: 2em 1.5em; position: relative; text-decoration: none; font-weight: normal; }"
".custom-file-upload:hover, a:hover { background-color: #ce0606; cursor: pointer; }"
".custom-file-upload.green, a.green { color: #474747; background-color: #00ee77; box-shadow: 0 5px 0 #03ad58; }"
".custom-file-upload.green:hover, a.green:hover { background-color: #03ad58; cursor: pointer; }"
".custom-file-upload:active, a:active { box-shadow: none; top: 5px; }"
"input[type=\"file\"] { display: none; }"
"</style>"
"</head><body>"
"<h1>Linuxkidd's RouterRebooter</h1><br />"
"<h3>Current state: <span='font-size: 0.75em;'>( Version: " VERSION " )</span></h3><hr><table class='state' width='600px'>"
"<tr><th width='350px'>WiFi SSID</th><td><input type='text' disabled id='wifi_name'></td></tr>"
"<tr><th>WiFi Strength</th><td><input type='text' disabled id='wifi_rssi'></td></tr>"
"<tr><th>Last Latency (ms)</th><td><input type='text' disabled id='lastms'/></td></tr>"
"<tr><th>Fail Count</th><td><input type='text' disabled id='countFail'/></td></tr>"
"<tr><th>Success Count<br />"
"<span style='font-weight: normal;'>Will only be &gt; 0 during recovery from prior failures.</span></th><td class='vertical-align: top;'><input type='text' disabled id='countSuccess'/></td></tr>"
"<tr><th>Resetting Router?</th><td><input type='text' disabled id='routerReset'/></td></tr>"
"<tr><th>Current Time</th><td><input type='text' disabled id='epoch'/></td></tr>"
"<tr><th>IP Address</th><td><input type='text' disabled id='ip'/></td></tr>"
"</table><form name='mySettingsForm' method='POST' action='/save' enctype='multipart/form-data'>"
"<h3>Settings</h3>"
"<table width='600px'>"
"<tr><th width='350px'>MQTT Server</th><td><input type='text' name='mqtt_server' id='mqtt_server'/></td></tr>"
"<tr><th>MQTT Topic Base</th><td><input type='text' name='mqtt_topic_base' id='mqtt_topic_base'/></td></tr>"
"<tr><td colspan='2'><hr /></td></tr>"
"<tr><th>Test Server</th><td><input type='text' name='test_server' id='test_server'/></td></tr>"
"<tr><th>Test Server Port</th><td><input type='text' name='test_port' id='test_port'/></td></tr>"
"<tr><th>Test Interval <span style='font-weight: normal;'>(seconds)</span></th><td><input type='text' name='test_interval' id='test_interval'/></td></tr>"
"<tr><th>Reset after X failures</th><td><input type='text' name='test_fail_reset' id='test_fail_reset'/></td></tr>"
"<tr><th>Connection healthy after X successes</th><td><input type='text' name='test_success_clear' id='test_success_clear'/></td></tr>"
"<tr><td colspan='2'><hr /></td></tr>"
"<tr><th>Router minimum on time <span style='font-weight: normal;'>(seconds)</span></th><td><input type='text' name='relay_min_on' id='relay_min_on'/></td></tr>"
"<tr><th>Router off time</th><td><input type='text' name='relay_off_time' id='relay_off_time'/></td></tr>"
"<tr><td colspan='2'><hr /></td></tr>"
"</table></form>"
"<div style='width: 600px; margin: 2em 0; text-align: center;'><a href='/restart'>Restart ESP</a> <a href='/off'>Reboot Router</a><a class='green' href=\"#\" onclick=\"document.forms['mySettingsForm'].submit(); return false;\">Save Settings</a></div><br /><br />"
"<div style='width: 600px; margin: 2em 0; text-align: center;'><form name='myUpdateForm' method='POST' action='/update' enctype='multipart/form-data'><label for=\"file-upload\" class=\"custom-file-upload green\">Select Firmware</label><input type='file' id=\"file-upload\" name='update'/> <a href=\"#\"  class='green' onclick=\"document.forms['myUpdateForm'].submit(); return false;\">Apply Firmware</a> <a href='/reset'>Factory Reset</a></form></div>"
"</body></html>";

const char* serverAPIndex = 
"<!doctype html> <html lang=\"en\"> <head> <meta charset=\"utf-8\"> <title>RouterRebooter</title>"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"> <meta name=\"viewport\" content=\"width=device-width,user-scalable=no\"> <meta name=\"description\" content=\"RouterRebooter\"> <meta name=\"author\" content=\"linuxkidd\">"
"<script type='text/javascript'>"
"window.onload = fetchStatus; "
"function fetchStatus() { fetch('/status').then((response) => { return response.json() }).then((data) => { "
"['lastms','countFail','countSuccess','routerReset','epoch','ip','mqtt_server','mqtt_topic_base','test_server','test_port','test_interval','test_fail_reset','test_success_clear','relay_min_on','relay_off_time'].forEach(function(item) { if(item in data) { document.getElementById(item).value = data[item]; }})}); setTimeout(fetchStatus,30000);}"
"</script>"
"<style>th { text-align: left; } input { text-align: right; }</style>"
"</head><body>"
"<h1>Linuxkidd's RouterRebooter</h1><br />"
"<h3>Join WiFi <span='font-size: 0.75em;'>( Version: " VERSION " )</span></h3>"
"<form name='myWiFiForm' method='POST' action='/savewifi' enctype='multipart/form-data'>"
"<table width='400'>"
"<tr><th width='150'>Your Network SSID:</th><td><input type='text' name='testssid' id='testssid'></td></tr>"
"<tr><th>Your Network Password:</th><td><input type='text' name='testpass' id='testssid'></td></tr>"
"<tr><th colspan='2' align='right'><input type='submit' value='Test and Save Connection'/></td></tr>"
"</table></form>"
"</body></html>";