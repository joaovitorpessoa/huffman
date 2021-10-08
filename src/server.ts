import https from "https";
import fs from "fs";
import express from "express";

const app = express();

const options = {
  key: fs.readFileSync("key.pem"),
  cert: fs.readFileSync("cert.pem"),
};

// app.use(express.);

app.post("/", (req, res) => {
  console.log(req.headers);
  console.log(req.body);

  res.send();
});

https
  .createServer(options, app)
  .listen(8000, () => console.log("Server is running on port 8000! 🚀"));
