const express = require("express");
const http = require("http");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");
const { Server } = require("socket.io");

const app = express();
const server = http.createServer(app);

// Инициализиране на серийния порт с обработка на грешки
let port;
try {
  port = new SerialPort({ 
    path: "COM3", // смени COM3 ако е нужно
    baudRate: 9600,
    autoOpen: false
  });
} catch (error) {
  console.error("Грешка при създаване на серийния порт:", error);
  process.exit(1);
}

// Отваряне на порта
port.open((err) => {
  if (err) {
    console.error("Грешка при отваряне на порта:", err);
    process.exit(1);
  }
  console.log("Серийният порт е отворен успешно");
});

const parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));
const io = new Server(server);

app.use(express.static("public"));
app.use(express.json());

let gameInProgress = false;

server.listen(3000, () => {
  console.log("Server running at http://localhost:3000");
});

app.post("/new-game", (req, res) => {
  if (!gameInProgress) {
    gameInProgress = true;
    port.write("NEW_GAME\n", (err) => {
      if (err) {
        console.error("Грешка при изпращане на NEW_GAME:", err);
        gameInProgress = false;
        return res.status(500).send("Грешка при комуникация с Arduino");
      }
      console.log("Изпратено: NEW_GAME");
      res.sendStatus(200);
    });
  } else {
    res.status(409).send("Игра вече е в ход");
  }
});

app.post("/button-press", (req, res) => {
  port.write("BUTTON_PRESSED\n", (err) => {
    if (err) {
      console.error("Грешка при изпращане на BUTTON_PRESSED:", err);
      return res.status(500).send("Грешка при комуникация с Arduino");
    }
    console.log("Изпратено: BUTTON_PRESSED");
    res.sendStatus(200);
  });
});

parser.on("data", data => {
  const message = data.trim();
  console.log("Arduino:", message);
  
  // Нулиране на флага за игра в ход при завършване
  if (message.startsWith("TIME:") || message === "TOO_EARLY") {
    gameInProgress = false;
  }
  
  io.emit("arduino", message);
});

// Обработка на грешки в серийния порт
port.on('error', (err) => {
  console.error('Грешка в серийния порт:', err);
});

port.on('close', () => {
  console.log('Серийният порт е затворен');
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('Затваряне на сървъра...');
  if (port && port.isOpen) {
    port.close(() => {
      console.log('Серийният порт е затворен');
      process.exit(0);
    });
  } else {
    process.exit(0);
  }
});