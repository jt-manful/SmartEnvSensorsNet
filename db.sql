CREATE DATABASE final_project;
use final_project;

-- Creation of SmartNodes Table
CREATE TABLE SmartNodes (
    NodeID INT AUTO_INCREMENT PRIMARY KEY,
    Name VARCHAR(255) NOT NULL,
    Location VARCHAR(255) NOT NULL
);

-- Creation of SensorTypes Table
CREATE TABLE SensorTypes (
    TypeID INT AUTO_INCREMENT PRIMARY KEY,
    TypeName VARCHAR(255) NOT NULL
);

-- Creation of SensorData Table
CREATE TABLE SensorData (
    DataID INT AUTO_INCREMENT PRIMARY KEY,
    NodeID INT,
    TypeID INT,
    Value DECIMAL(10,2) NOT NULL,
    Timestamp DATETIME NOT NULL,
    FOREIGN KEY (NodeID) REFERENCES SmartNodes(NodeID),
    FOREIGN KEY (TypeID) REFERENCES SensorTypes(TypeID)
);

-- Creation of SensorData Table
CREATE TABLE MLDATA (
    DataID INT AUTO_INCREMENT PRIMARY KEY,
    NodeID INT,
    Temperature DECIMAL,
    LDR DECIMAL,
    Humidity DECIMAL,
    Timestamp DATETIME NOT NULL,
    FOREIGN KEY (NodeID) REFERENCES SmartNodes(NodeID),
    FOREIGN KEY (TypeID) REFERENCES SensorTypes(TypeID)
);
