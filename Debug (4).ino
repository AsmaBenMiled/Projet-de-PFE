#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <FS.h>
#include <vector>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>


ESP8266WebServer server(80);

/* ======================= Sensor configuration ======================= */
#define ONE_WIRE_BUS 0  // Data wire is connected to digital pin 7 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);  // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire); // Pass oneWire reference to DallasTemperature library
/* ======================= NTP Configuration ======================= */
const char* ntpServer = "pool.ntp.org";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer );

String currentTime;
String currentDay;
String currentDate ;
/* ======================= GLOBAL VARIABLES ======================= */
float temp_sensor = 0;  //sensor temperature
unsigned long current_time = millis(); // Get the current time in milliseconds
int Is_First_consigne = 0 ;
int consigne = 0;     //"Consigne" Value entered by the client
unsigned long last_consigne_update = current_time + 60000; // Initialize a variable to keep track of the last time "consigne" was updated
String  check_mode = "" ; 
bool ALLOW = false ;
int i = 0 ; // runs the the events table rows
String formattedDate;

String ssid = "";
String password = "";
const char* ssid_def= "TOPNET_FEF0";
const char* password_def = "ErTCf25ze21";




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function is called in the getDataRows function to divide the lines of data in the data file in SPIFFS according to the placement of the "," in each line
std::vector<String> splitString(String input, String delimiter) {
  std::vector<String> output; int index = 0;
  while (index >= 0) {
    int nextIndex = input.indexOf(delimiter, index);
    if (nextIndex < 0) {
      output.push_back(input.substring(index));
      index = nextIndex;
    } else {
      output.push_back(input.substring(index, nextIndex));
      index = nextIndex + delimiter.length();
    }
  } return output;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function sorts data vector by days and by hours
std::vector<std::vector<String>> GET_sorted_data( std::vector<std::vector<String>> data ) {
      
      File dataFile = SPIFFS.open("/data.txt", "r");    // Read the data from the file and store it in a vector of vectors
  if (dataFile) {
    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n'); line.trim();
      if (line.length() > 0) {
        std::vector<String> fields = splitString(line, ",");
        data.push_back(fields);
      }
    }
    dataFile.close();
  }
  // Sort the data by days and hours
  std::sort(data.begin(), data.end(), [](std::vector<String> a, std::vector<String> b) {
    if (a[1] != b[1]) {  // Sort by days
      std::vector<String> days = {"Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"};
      auto a_day = std::find(days.begin(), days.end(), a[1]); auto b_day = std::find(days.begin(), days.end(), b[1]); return a_day < b_day;
    } else {
      /* Sort by hours */  return a[2] < b[2];
    }
  });
        return data ;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function adds an event in the data file with the entered parameters
void ADD( std::vector<String> days, String heure, String temp ) {
  // Validate the form data :
  if (days.size() == 0 || heure.length() == 0 || temp.length() == 0) {
    /* if One or more fields are empty */ server.send(400, "text/plain", "Please fill in all fields.");
  }
  // Combine the form data into a single string and Generate a unique ID for each day to store it in the data file :
  String formData = "";
  for (int i = 0; i < days.size(); i++) {
    String id = String(random(10000, 99999)) + String(random(10000, 99999));
    formData += id + "," + days[i] + "," + heure + "," + temp + "\n";
  }

  // Open the data file in "append" mode and append the data
  File dataFile = SPIFFS.open("/data.txt", "a");
  if (dataFile) {
    dataFile.println(formData); dataFile.close();
    server.sendHeader("Location", "/chart"); server.send(302);
  } else {
    server.send(500, "text/plain", "Error saving data");
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function delete an event from the data file with a matching id
void DELETE(String id) {
       if(ALLOW == true){
         File dataFile = SPIFFS.open("/data.txt", "r");     /*<==== Open the data file for reading */  if (!dataFile) {
    server.send(500, "text/plain", "Error opening file " + String("/data.txt"));
    return;
  }
  // =======> Read the file line by line, skipping the line that matches the ID
  String newData = "";
  while (dataFile.available())    {
    String line = dataFile.readStringUntil('\n'); line.trim();
    if (line.length() > 0)    {
      std::vector<String> fields = splitString(line, ",");
      if (fields[0] != id) {
        newData += line + "\n";
      }
    }
  }
  dataFile.close();
  // =======> Open the data file for writing
  dataFile = SPIFFS.open("/data.txt", "w");
  if (!dataFile) {
    server.send(500, "text/plain", "Error opening file " + String("/data.txt"));
    return;
  }
  // =======> Write the modified content back to the file
  dataFile.print(newData);  dataFile.close();
        }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");} 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This Part has functions that send the css code to any html page with the matching path
void handleDesign(String path) {
  File cssFile = SPIFFS.open(path, "r"); if (cssFile) {
    String cssContent = cssFile.readString();
    if(check_mode != "AUTO"){cssContent.replace("/*.modeAuto{display:none;}*/",".modeAuto{display:none;}");}
    cssFile.close();
    server.send(200, "text/css", cssContent);
  } else {
    server.send(404, "text/plain", "File not found");
  }
}
void handleDesignIDX()  {
  handleDesign("/index.css");
} ; void handleDesignIDXX()  {
  handleDesign("/common_style.css");
} ; void handleDesignParam() {
  handleDesign("/Parameters.css");
} ;  void handleDesignAdd()   {
  handleDesign("/eventAj.css");
} ; void handleDesignchart() {
    handleDesign("/chart.css");
} ;  void handleDesignlog() {
    handleDesign("/login.css");
} 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  This function is called every minute in the void loop to assign a value to the "consigne" variable : "consigne" presents the temperature entered by the client in the events table
//     ||
//     \/
//                    *******************************************************************
int  GET_consigne() {
  int Value = 23 ; // value by default 
  String Mode;  // ON / OFF / AUTO
  std::vector<std::vector<String>> data ;  /* PUT the data in a vector and sort them by days and hours ==> */ data = GET_sorted_data(data) ;

  if ((data[0][1] == currentDay) && (data[0][2] == currentTime)) { // Assigns the first temp to "consigne"
    Is_First_consigne = 1 ; // to prevent the assignement of first table temp to the consigne variable if it's not time to
    String val_con = data[0][3]; Value = val_con.toInt();
  }
  if ((data[i+1][1] == currentDay) && (data[i+1][2] == currentTime)) { // checking if the next row of the table is identical to currentTime and currentDay
    String val_con = data[i+1][3]; Value = val_con.toInt();  i=i+1;
  } else {
    if (Is_First_consigne == 1) {
      String val = data[i][3] ;
      Value = val.toInt();
    }
  }

  // Change LED State if it's auto mode
  std::vector<String> modes;
  File file = SPIFFS.open("/mode.txt", "r");  // GET the mode.txt where the modes are stocked
  if (file) {
    while (file.available()) {
      String Mode = file.readStringUntil('\n');
      modes.push_back(Mode);
    }
    file.close();
  }
  if (modes.size() > 0) {
    Mode = modes.back();   //Assign the last value in the vector to Mode
    Mode.trim();
  }
  if (Mode == "AUTO") {
    if (Value > temp_sensor) {
      digitalWrite(2, LOW); /* LED Turn ON*/
    } else {
      digitalWrite(2, HIGH);  /* LED Turn OFF*/
    }
  }

  Serial.println("\n\n Now is =  " + currentDay + "  at:  " + currentTime + "\t with value =\t" + Value );
  return Value;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //  This function is called to deliver the "/index" page with the correct changes that matches the current mode  
   //     ||
   //     \/
   //                    *******************************************************************
void handleRoot() {
        Serial.println("\nHandleRoot entered");
  
  String Mode;
  // =====> the first part checks if there's any button clicked ( mode activated ) and append the mode to a file in memory
if (server.method() == HTTP_POST) {
  String OnVal = server.arg("modeon"); String OffVal = server.arg("modeoff"); String autoVal = server.arg("modeauto");
  String Mode_Name;
  File file = SPIFFS.open("/mode.txt", "a"); if (file) { if (OnVal == "1") { Mode_Name = "ON"; } else if (OffVal == "1") { Mode_Name = "OFF"; } else if (autoVal == "1") { Mode_Name = "AUTO";} file.println(Mode_Name); file.close();}
}
  // =====> this part run the lines of mode.txt where the modes are stocked and put it in a vector so later we can extract the mode name in the last line
std::vector<String> modes;
File file = SPIFFS.open("/mode.txt", "r");
if(file) { while (file.available()) { String Mode = file.readStringUntil('\n'); modes.push_back(Mode); } file.close(); }  
  //Assign the last value in the vector to Mode
if (modes.size() > 0) { Mode = modes.back(); Mode.trim(); check_mode = Mode;} 

  // ======> CSS & Html changes for each mode 
  String html;
  String css;
Serial.println("MODE IS =" + Mode);
 file = SPIFFS.open("/index.html", "r");if(file){ html = file.readString(); file.close();}else{Serial.println("M1= can't open file html 1  " );} // read html content
 file = SPIFFS.open("/index.css", "r"); if(file) { css = file.readString(); file.close(); }else { Serial.println("can't open file css 1  "  );}  // read css content
if ((Mode == "ON") || (Mode == "AUTO") ){ 
     if (Mode == "ON") { digitalWrite(2, LOW);
         css.replace("/*on*/", "background: rgb(149, 241, 171);");
         css.replace("/*cardName ON color*/","color: var(--white);");
         css.replace("/*blueon img*/","filter: invert(100%) sepia(100%) hue-rotate(180deg) brightness(1000%);");
         css.replace("/*.modeAuto{display:none;}*/",".modeAuto{display:none;}");
         css = "<style>"+css+"</style></head>"; // make button green
         html.replace("<div class='cardName'>Activer</div>", "<div class='cardName'>Activé</div>"); 
                       } else{
                        if(consigne > temp_sensor){ digitalWrite(2, LOW); } else {digitalWrite(2, HIGH);} 
                          css.replace("/*auto*/", "background: rgb(34, 213, 213);");
                          css.replace("/*cardName AUTO color*/","color: var(--white);");
                          css.replace("/*blueauto img*/","filter: invert(100%) sepia(100%) hue-rotate(180deg) brightness(1000%);");
                          css = "<style>"+css+"</style></head>";
                             }  html.replace("</head>",css);
                                          } else { // Mode = off
                                             digitalWrite(2, HIGH); /* turn off LED connected to pin 2*/
                                             css.replace("/*off*/", "background: rgb(255, 143, 143);");
                                             css.replace("/*cardName OFF color*/","color: var(--white);");
                                             css.replace("/*blueoff img*/","filter: invert(100%) sepia(100%) hue-rotate(180deg) brightness(1000%);");
                                             css.replace("/*.modeAuto{display:none;}*/",".modeAuto{display:none;}");
                                             css = "<style>"+css+"</style></head>";
                                              html.replace("<div class='cardName'>Désactiver</div>", "<div class='cardName'>Désactivé</div>"); 
                                              html.replace("</head>",css);
                                              } html.replace("<!-- Sensor Value -->",String(temp_sensor)); html.replace("<!-- Consigne Value -->",String( consigne) );  html.replace("<!-- PUT TIME-->",String(currentTime));html.replace("<!--Replace Date-->",String(currentDate));
         if(ALLOW == true){
          // Send the updated HTML and CSS files to the browser
             server.send(200, "text/html", html);
          }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
   }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function sends the html code of "/Param" page
void handleParameter() {
    if(ALLOW == true){
       File htmlFileParam = SPIFFS.open("/Param.html", "r");   // Read the HTML file from the SPIFFS file system
  if (htmlFileParam) {
    String htmlContentParam = htmlFileParam.readString();
    htmlContentParam.replace("<!-- PUT TIME-->",String(currentTime));htmlContentParam.replace("<!--Replace Date-->",String(currentDate));
    
    htmlFileParam.close(); /* Send the HTML page with the form */server.send(200, "text/html", htmlContentParam);
  } else {
    Serial.println("Error opening file ");
    server.send(404, "text/plain", "File not found");
  }
      }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlelogin() {
   File htmlFilelogin = SPIFFS.open("/login.html", "r");   // Read the HTML file from the SPIFFS file system
  if(htmlFilelogin) {
    String htmlContentlogin = htmlFilelogin.readString();    
    htmlFilelogin.close(); /* Send the HTML page with the form */server.send(200, "text/html", htmlContentlogin);
  } else {    server.send(404, "text/plain", "File not found");}
  }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This function sends the html code of "/Ajout" page
void handleForm() {
     if(ALLOW == true){
       if (server.method() == HTTP_GET) {
    File htmlFileForm = SPIFFS.open("/Ajout.html", "r");
    if (htmlFileForm) {
      String htmlContentForm = htmlFileForm.readString();
      htmlContentForm.replace("<!-- PUT TIME-->",String(currentTime));htmlContentForm.replace("<!--Replace Date-->",String(currentDate));
      htmlFileForm.close();
      server.send(200, "text/html", htmlContentForm);
    } else {
      server.send(404, "text/plain", "File not found");
    }
  }
      }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handleChart() {
  if(ALLOW == true){
     std::vector<std::vector<String>> data ;  /* PUT the data in a vector and sort them by days and hours ==> */ data =  GET_sorted_data(data) ;
    std::vector<String> labels ; 
  // Prepare the times values for Yaxis of the chart
    // Initialize an array for each day of the week
  String Lundi = "var Lundi = [  ";
  String Mardi = "var Mardi = [  ";
  String Mercredi = "var Mercredi = [  ";
  String Jeudi = "var Jeudi = [  ";
  String Vendredi = "var Vendredi = [  ";
  String Samedi = "var Samedi = [  ";
  String Dimanche = "var Dimanche = [  ";
  String labels_h = "var labels_h =[  ";
  for (int i = 0; i < data.size(); i++){
    if(data[i][1] == "Lundi") {Lundi += "'" + data[i][2] + "', ";   labels.push_back(data[i][2]) ; }
    if (data[i][1] == "Mardi") { Mardi += "'" + data[i][2] + "', "; labels.push_back(data[i][2]) ;}
    if (data[i][1] == "Mercredi") {Mercredi += "'" + data[i][2] + "', "; labels.push_back(data[i][2]) ;}
    if (data[i][1] == "Jeudi") { Jeudi += "'" + data[i][2] + "', " ;labels.push_back(data[i][2]) ; }
    if (data[i][1] == "Vendredi") { Vendredi += "'" + data[i][2] + "', ";labels.push_back(data[i][2]) ;}
    if (data[i][1] == "Samedi") {Samedi += "'" + data[i][2] + "', ";labels.push_back(data[i][2]) ;}
    if (data[i][1] == "Dimanche") {Dimanche += "'" + data[i][2] + "', ";labels.push_back(data[i][2]) ;}
    }
  std::sort(labels.begin(), labels.end());
  auto last = std::unique(labels.begin(), labels.end());
  labels.erase(last, labels.end());
  
  for (int i = 0; i < labels.size(); i++){
    labels_h += "'" + labels[i] + "', " ;
    }
  // Remove the trailing comma and space from each array
  Lundi = Lundi.substring(0, Lundi.length() - 2) + "];";Mardi = Mardi.substring(0, Mardi.length() - 2) + "];";Mercredi = Mercredi.substring(0, Mercredi.length() - 2) + "];";Jeudi = Jeudi.substring(0, Jeudi.length() - 2) + "];";Vendredi = Vendredi.substring(0, Vendredi.length() - 2) + "];";Samedi = Samedi.substring(0, Samedi.length() - 2) + "];";Dimanche = Dimanche.substring(0, Dimanche.length() - 2) + "];";
  labels_h = labels_h.substring(0, labels_h.length() - 2) + "];";
  
  //Serial.println(String(Lundi) + "\n" +String(Mardi) + "\n"+String(Mercredi) + "\n"+String(Jeudi) + "\n"+String(Vendredi) + "\n"+String(Samedi) + "\n"+String(Dimanche) + "\n"+String(labels_h) + "\n"  );

  // Prepare the temperature values for the Xaxis of chart : 
  String temp_1 = "[  " ; String temp_2 = "[  " ;String temp_3 = "[  " ;String temp_4 = "[  " ;String temp_5 = "[  " ;String temp_6 = "[  " ;String temp_7 = "[  " ;
  
  String ID_1 = "var ID_1=[  " ; String ID_2 = "var ID_2 = [  " ;String ID_3 = "var ID_3 = [  " ;String ID_4 = "var ID_4 = [  " ;String ID_5 = "var ID_5 = [  " ;String ID_6 = "var ID_6 = [  " ;String ID_7 = "var ID_7 = [  " ;
  

for (int i = 0; i < labels.size(); i++) {
    int j = 0;
    int found = 0;
    while ((j < data.size()) && (found == 0)) { if ((data[j][1] == "Lundi") && (data[j][2] == labels[i])) { temp_1 += data[j][3] + ", ";ID_1 += data[j][0] + ", "; found = 1; } j++ ; }
    if (found == 0) {temp_1 += "null, "; ID_1 += "0, ";}
        
       j = 0; found = 0;
    while ((j < data.size()) && (found == 0)) { if ((data[j][1] == "Mardi") && (data[j][2] == labels[i])) { temp_2 += data[j][3] + ", ";ID_2 += data[j][0] + ", "; found = 1; } j++ ;}
    if (found == 0) {temp_2 += "null, ";ID_2 += "0, ";}
    
      j = 0; found = 0;
    while ((j < data.size()) && (found == 0)) { if ((data[j][1] == "Mercredi") && (data[j][2] == labels[i])) { temp_3 += data[j][3] + ", ";ID_3 += data[j][0] + ", "; found = 1; } j++ ;}
    if (found == 0) {temp_3 += "null, ";ID_3 += "0, ";}
    
          j = 0; found = 0;
    while ((j < data.size()) && (found == 0)) { if ((data[j][1] == "Jeudi") && (data[j][2] == labels[i])) { temp_4 += data[j][3] + ", ";ID_4 += data[j][0] + ", "; found = 1; } j++ ;}
    if (found == 0) {temp_4 += "null, ";ID_4 += "0, ";}

    j = 0; found = 0;
    while ((j < data.size()) && (found == 0)) {if ((data[j][1] == "Vendredi") && (data[j][2] == labels[i])) { temp_5 += data[j][3] + ", ";ID_5 += data[j][0] + ", "; found = 1; } j++ ;}
    if (found == 0) {temp_5 += "null, ";ID_5 += "0, ";}

          j = 0; found = 0;
    while ((j < data.size()) && (found == 0)) {if ((data[j][1] == "Samedi") && (data[j][2] == labels[i])) { temp_6 += data[j][3] + ", ";ID_6 += data[j][0] + ", "; found = 1; } j++ ;}
    if (found == 0) {temp_6 += "null, ";ID_6 += "0, ";}

          j = 0; found = 0;
    while ((j < data.size()) && (found == 0)) {if ((data[j][1] == "Dimanche") && (data[j][2] == labels[i])) { temp_7 += data[j][3] + ", ";ID_7 += data[j][0] + ", "; found = 1; } j++ ;}
    if (found == 0) {temp_7 += "null, ";ID_7 += "0, ";}
    
        }

  // Remove the trailing comma and space from each array
  temp_1 = temp_1.substring(0, temp_1.length() - 2) + "]";temp_2 = temp_2.substring(0, temp_2.length() - 2) + "]";temp_3 = temp_3.substring(0, temp_3.length() - 2) + "]";temp_4 = temp_4.substring(0, temp_4.length() - 2) + "]";temp_5 = temp_5.substring(0, temp_5.length() - 2) + "]";temp_6 = temp_6.substring(0, temp_6.length() - 2) + "]";temp_7 = temp_7.substring(0, temp_7.length() - 2) + "]";
  ID_1 = ID_1.substring(0, ID_1.length() - 2) + "];";
  ID_2 = ID_2.substring(0, ID_2.length() - 2) + "];";
  ID_3 = ID_3.substring(0, ID_3.length() - 2) + "];";
  ID_4 = ID_4.substring(0, ID_4.length() - 2) + "];";
  ID_5 = ID_5.substring(0, ID_5.length() - 2) + "];";
  ID_6 = ID_6.substring(0, ID_6.length() - 2) + "];";
  ID_7 = ID_7.substring(0, ID_7.length() - 2) + "];";

//Serial.println(String(temp_1) + "\n" + String(temp_2) + "\n" + String(temp_3)+ "\n" + String(temp_4)+ "\n" + String(temp_5)+ "\n" + String(temp_6)+ "\n" + String(temp_7));

//Serial.println(String(ID_1) + "\n" + String(ID_2) + "\n" + String(ID_3)+ "\n" + String(ID_4)+ "\n" + String(ID_5)+ "\n" + String(ID_6)+ "\n" + String(ID_7));




  
  File htmlFilechart = SPIFFS.open("/chart.html", "r");
    if (htmlFilechart) {
      String htmlContentchart = htmlFilechart.readString();
      htmlFilechart.close();
      htmlContentchart.replace("/*Lundi*/",Lundi);htmlContentchart.replace("/*Mardi*/",Mardi);htmlContentchart.replace("/*Mercredi*/",Mercredi);htmlContentchart.replace("/*Jeudi*/",Jeudi);htmlContentchart.replace("/*Vendredi*/",Vendredi);htmlContentchart.replace("/*Samedi*/",Samedi);htmlContentchart.replace("/*Dimanche*/",Dimanche);
      htmlContentchart.replace("/*labels_h*/",labels_h);     htmlContentchart.replace("<!-- PUT TIME-->",String(currentTime));htmlContentchart.replace("<!--Replace Date-->",String(currentDate));
      htmlContentchart.replace("/*temp1*/",temp_1);htmlContentchart.replace("/*temp2*/",temp_2);htmlContentchart.replace("/*temp3*/",temp_3);htmlContentchart.replace("/*temp4*/",temp_4);htmlContentchart.replace("/*temp5*/",temp_5);htmlContentchart.replace("/*temp6*/",temp_6);htmlContentchart.replace("/*temp7*/",temp_7);
      htmlContentchart.replace("/*ID_1*/",ID_1); htmlContentchart.replace("/*ID_2*/",ID_2); htmlContentchart.replace("/*ID_3*/",ID_3); htmlContentchart.replace("/*ID_4*/",ID_4);htmlContentchart.replace("/*ID_5*/",ID_5);htmlContentchart.replace("/*ID_6*/",ID_6);htmlContentchart.replace("/*ID_7*/",ID_7);
 server.send(200, "text/html", htmlContentchart);
    } else {
      server.send(404, "text/plain", "File not found");
    } 
}else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handle_Acces(){
    if (server.method() == HTTP_POST) {
    if (server.hasArg("user") && server.hasArg("passw")) {
      String USER = server.arg("user"); String PASS = server.arg("passw");
      if((USER == "admin") &&(PASS == "tempuser2023") ){
        ALLOW = true ;
        server.sendHeader("Location", String("/index")); server.send(302, "text/plain", ""); 
        }else{
                  server.sendHeader("Location", String("/")); server.send(302, "text/plain", ""); 
          }
      }
    }
  }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlelogout(){ALLOW = false;server.sendHeader("Location", String("/")); server.send(302, "text/plain", ""); }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  This function is called to manages the submitted data in the "/Ajout" page
void handleFormData() {
  if(ALLOW == true){
    if (server.method() == HTTP_POST) {
    if (server.hasArg("heure") && server.hasArg("temp")) {
      String heure = server.arg("heure");   String temp = server.arg("temp");   temp.trim();   heure.trim();
      std::vector<String> days;  // declare a vector
      for (int i = 0; i < 7; i++) { // check which days are recieved in the server
        if (server.hasArg("days[" + String(i) + "]")) {
          String day = server.arg("days[" + String(i) + "]");
          day.trim();
          days.push_back(day);
        }
      }
      ADD(days, heure, temp);
    } else {
      server.send(400, "text/plain", "Missing form fields");
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
  }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This  function deletes a row in the table
void Delete_Event() {
  if (server.hasArg("delete-id")) {
    String id = server.arg("delete-id");
    DELETE(id) ; // call delete function
    // =======> Redirect to the index page
    server.sendHeader("Location", String("/chart")); server.send(302, "text/plain", "");
  } else {
    server.send(400, "text/plain", "Missing ID parameter");
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This  function delivers the html code of "/modify" page with the data of the row to modify pre-selected and filled
//          ||
//          \/
//             *******************************************************
void handleForm_MODIFY() {
    if(ALLOW == true){
      if (server.hasArg("modify-id")) {

    String id = server.arg("modify-id"); // Get the ID parameter from the request
    Serial.println(String(id));
    // =====>  Open the data file for reading
    File dataFile = SPIFFS.open("/data.txt", "r");
    if (!dataFile) {
      server.send(500, "text/plain", "Error opening file " + String("/data.txt"));
      return;
    }
    // =====> Find the event with the matching ID and extract its data
    String days = ""; String heure = ""; String temp = "";
    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n'); line.trim();
      if (line.length() > 0) {
        std::vector<String> fields = splitString(line, ",");
        if (fields[0] == id) {
          days = fields[1];
          heure = fields[2];
          temp = fields[3];
          break;
        }
      }
    } dataFile.close();
    File htmlFilemod = SPIFFS.open( "/modify.html", "r");      // Read the HTML file from the SPIFFS file system
    if (htmlFilemod) {
      String htmlContentmod = htmlFilemod.readString(); htmlFilemod.close();
      // Replace the placeholder with the correspendant data
      String days_checkboxes = "";   String days_of_week[7] = {"Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"};
      for (int i = 0; i < 7; i++) {
        String day = days_of_week[i];
        days_checkboxes += "<label><input type='checkbox' name='days[" + String(i) + "]' id='day" + String(i + 1) + "' value='" + day + "'";
        if (days.indexOf(day) != -1) {
          days_checkboxes += " checked";
        } days_checkboxes += ">" + day + "</label>";
      }
      // Replace the placeholder with the concatenated checkboxes string
      htmlContentmod.replace("<!-- TO BE REPLACED DAYS -->", days_checkboxes); htmlContentmod.replace("REPLACE_HEURE", heure); htmlContentmod.replace("REPLACE_TEMP", temp); htmlContentmod.replace("MODIFY_ID", id); htmlContentmod.replace("<!-- PUT TIME-->",String(currentTime));htmlContentmod.replace("<!--Replace Date-->",String(currentDate));

      // =====> Render the modify page and pass the data as template variables
      server.sendHeader("Content-Type", "text/html"); server.send(200, "text/html", htmlContentmod);
    }
  } else {
    server.send(400, "text/plain", "Missing ID parameter");
  }
      }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This  function changes the data in the dataFile if any event in the table is modified
// We delete the line of the row modified and we add the whole new data updated
void handleFormData_MODIFY() {
    if(ALLOW == true){
      if (server.method() == HTTP_POST) {
    // ====> Check if the request has the necessary form fields
    if (server.hasArg("heure") && server.hasArg("temp") && server.hasArg("id_m")) {
      String heure  = server.arg("heure"); String  temp  = server.arg("temp"); String id_mod = server.arg("id_m"); temp.trim();  heure.trim();
      std::vector<String> days;
      for (int i = 0; i < 7; i++) {
        if (server.hasArg("days[" + String(i) + "]")) {
          String day = server.arg("days[" + String(i) + "]");
          day.trim();
          days.push_back(day);
        }
      }

      DELETE(id_mod); // delete the row from the data file where id = id_mod
      ADD(days, heure, temp); // add the new row to the data file with the new data
    } else {
      server.send(400, "text/plain", "Missing form fields");
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
      }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handle_Config() {
    if(ALLOW == true){
      if (server.method() == HTTP_POST) {
    if (server.hasArg("identifiant") && server.hasArg("motdepasse")) {
      ssid = server.arg("identifiant");   password = server.arg("motdepasse");   ssid.trim();   password.trim();}
      Serial.println("ssid  : " + String(ssid)  +"\n" + "password  : " + String(password) );
      Serial.println("Attempting to connect to your new Wi-Fi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      int attempts = 0; int max_attempts = 10;
  while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to  : " + String(ssid));
      Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed ! reconnecting to WiFi.");
     WiFi.begin(ssid_def, password_def);
    while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to : " + String(ssid_def));
    Serial.println(WiFi.localIP());
    
  }
      server.sendHeader("Location", "/index"); server.send(302);
     } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
      }else {server.send(400, "text/plain", "NO ACCESS ALLOWED !");}
  }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// SETUP         &&           LOOP/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  sensors.begin();  // Start up the library
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH); /* turn off LED  at the start of the system*/

  // Mount the SPIFFS file system
  if (!SPIFFS.begin()) {
    Serial.println("Error mounting SPIFFS file system");
    return;
  }

  // if (!ssid){WiFi.begin("TOPNETFEF0","pwd")} else {WiFi.begin(ssid, password);}
  // Connect to Wi-Fi
  WiFi.begin(ssid_def, password_def);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to : " + String(ssid_def));
    timeClient.begin(); 
    timeClient.setTimeOffset(3600);
  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());
  // Start the web server
  server.begin();
  // Register the root path handler
  server.on("/",handlelogin);                             server.on("/login.css", handleDesignlog);                      server.on("/log_in",HTTP_POST,handle_Acces);
  server.on("/index",handleRoot);              server.on("/index.css", handleDesignIDX); server.on("/common_style.css", handleDesignIDXX);
  server.on("/Param",HTTP_GET, handleParameter);          server.on("/Parameters.css", HTTP_GET , handleDesignParam);    server.on("/handle_ssid", HTTP_POST,handle_Config);
  server.on("/Ajout", HTTP_GET , handleForm);             server.on("/eventAj.css", HTTP_GET , handleDesignAdd);         server.on("/submit", HTTP_POST, handleFormData);
  server.on("/modify", HTTP_POST , handleForm_MODIFY);                                                                   server.on("/save", HTTP_POST, handleFormData_MODIFY);
  server.on("/delete", HTTP_POST, Delete_Event);      
  server.on("/chart",HTTP_GET, handleChart);              server.on("/chart.css", HTTP_GET , handleDesignchart);
  server.on("/logout",handlelogout);
  // Serve the Highcharts.js file
  server.on("/highcharts.js", HTTP_GET, []() {
    File file = SPIFFS.open("/highcharts.js", "r");
    if (!file) {
      server.send(404, "text/plain", "File not found");
      return;
    }
    server.streamFile(file, "application/javascript");
    file.close();});   
}

void loop() {
  server.handleClient();
   timeClient.update();
String weekDays[7] = {"Dimanche","Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi"};
String currentHour ;
int Hour = timeClient.getHours();
if(Hour<=9) {currentHour= "0"+String(Hour);} else {currentHour= String(Hour);}
int currentMinute = timeClient.getMinutes();
if(currentMinute<=9){ currentTime = String(currentHour) + ":0" + String(currentMinute);}else {currentTime = String(currentHour) + ":" + String(currentMinute);}
currentDay = weekDays[timeClient.getDay()];

 time_t epochTime = timeClient.getEpochTime();
 //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;

  //Print complete date:
    currentDate = String(currentDay)+ "  " +String(currentYear) + "/" + String(currentMonth) + "/" + String(monthDay);
     /////////////////////////////////////////////////////////////////////////////////////////////////
  unsigned long current_time = millis(); // Get the current time in milliseconds

  if (current_time - last_consigne_update >= 60000) {   // Check if a minute has elapsed since the last time consigne was updated to execute the GET_consigne function
      sensors.requestTemperatures(); // Send the command to get temperatures
      //print the temperature in Celsius
      Serial.print("\n Temperature: ");
      Serial.print(sensors.getTempCByIndex(0));Serial.print("°C");
      temp_sensor = sensors.getTempCByIndex(0);
    std::vector<std::vector<String>> data;
    data =  GET_sorted_data(data) ;
    if (i >= data.size()-1){
      /* check if we are in the last "consigne" in the table so the last value of the variable stays permanant*/  String get_cons = data[i][3]; consigne = get_cons.toInt();
    } else {
      consigne = GET_consigne(); /* Update consigne value every minute */
    }
    last_consigne_update = current_time; // Update the last update time
  }
  delay(1000);
}
