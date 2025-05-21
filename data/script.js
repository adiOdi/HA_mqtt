const preset = {
  spaces: [
    { x: 0, y: 0, width: 429, height: 452 },
    { x: 46, y: 302, width: 114, height: 83 },
    { x: 132, y: 333, width: 116, height: 52 },
    { x: 337, y: 337, width: 86, height: 128 },
    { x: 2, y: 74, width: 160, height: 93 },
    { x: 307, y: 189, width: 118, height: 93 },
    { x: 145, y: -10, width: 243, height: 107 },
  ],
  sensor: { x: 420, y: 78, rot: 2.3 },
};

var sensor = preset.sensor;

let person = {
  x: 105,
  y: 360,
};

let spaces = preset.spaces;

fetch("/preset.json")
  .then((response) => response.json())
  .then((response) => {
    console.log("preset is", response);
    spaces = response.spaces;
    sensor = response.sensor;
    console.log("spaces are", spaces);
    console.log("sensor is", sensor);
    redrawAll();
  });
// 0 is sensor, 1...9 is rects
let editMode = -1;

const scale = 1;

var c = document.getElementById("myCanvas");
var ctx = c.getContext("2d");

var drawing = false;
//start the drawing if the mouse is down
document.addEventListener("mousedown", (event) => {
  fetch("/data")
    .then((response) => response.json())
    .then((response) => {
      person = response;
      console.log(person);
      redrawAll();
    });
  drawing = true;
  const mouse = getMousePos(event);
  const space = spaces[editMode - 1];
  if (editMode > 0) {
    space.x = mouse.x;
    space.y = mouse.y;
  }
  // if (drawing && editMode < 0) {
  //   person.x = mouse.x;
  //   person.y = mouse.y;
  //   redrawAll();
  // }
});
//stop the drawing if the mouse is up
document.addEventListener("mouseup", (event) => {
  drawing = false;
  const mouse = getMousePos(event);
  const space = spaces[editMode - 1];
  if (editMode > 0) {
    if (space.y < mouse.y) space.height = Math.round(mouse.y - space.y);
    else {
      space.height = Math.round(space.y - mouse.y);
      space.y = mouse.y;
    }
    if (space.x < mouse.x) space.width = Math.round(mouse.x - space.x);
    else {
      space.width = Math.round(space.x - mouse.x);
      space.x = mouse.x;
    }
    redrawAll();
    updateConfig();
  }
});
//add an event listener to the canvas for when the user moves the mouse over it and the mouse is down
document.addEventListener("mousemove", (event) => {
  const mouse = getMousePos(event);
  if (drawing && editMode < 0) {
    person.x = mouse.x;
    person.y = mouse.y;
    redrawAll();
  }
  const space = spaces[editMode - 1];
  //if the drawing mode is true (if the mouse button is down)
  if (drawing == true && editMode > 0) {
    space.height = mouse.y - space.y;
    space.width = mouse.x - space.x;
    ctx.beginPath();
    ctx.fillStyle = "rgb(250 0 0 / 10%)";
    ctx.fillRect(space.x, space.y, space.width, space.height);
    ctx.fill();
  }
  if (!editMode) {
    sensor.x = mouse.x;
    sensor.y = mouse.y;
    redrawAll();
  }
});

document.addEventListener("keypress", (event) => {
  // console.log(event.key);
  switch (event.key) {
    case "s":
      editMode = 0;
      console.log("edit sensor now");
      break;
    case "a":
      if (!editMode) {
        sensor.rot -= 0.1;
        redrawAll();
        console.log("rotate sensor now");
        updateConfig();
      }
      break;
    case "d":
      if (!editMode) {
        sensor.rot += 0.1;
        redrawAll();
        console.log("rotate sensor now");
        updateConfig();
      }
      break;
    case "0":
      editMode = 0;
      console.log("edit sensor now");
      break;
    case " ":
      editMode = -1;
      console.log("disable editing");
      break;
    case "1":
      editMode = 1;
      console.log("edit rect 1 now");
      break;
    case "2":
      editMode = 2;
      console.log("edit rect 2 now");
      break;
    case "3":
      editMode = 3;
      console.log("edit rect 3 now");
      break;
    case "4":
      editMode = 4;
      console.log("edit rect 4 now");
      break;
    case "5":
      editMode = 5;
      console.log("edit rect 5 now");
      break;
    case "6":
      editMode = 6;
      console.log("edit rect 6 now");
      break;
    case "7":
      editMode = 7;
      console.log("edit rect 71 now");
      break;
    default:
      break;
  }
  redrawAll();
});

const img = new Image();

img.addEventListener("load", () => {
  redrawAll();
});

img.src = "Board.svg";

function drawSensor(sensor) {
  ctx.save();
  ctx.translate(sensor.x, sensor.y);
  ctx.rotate(sensor.rot);
  ctx.lineWidth = 1;
  ctx.strokeStyle = "rgb(0 0 0 / 70%)";
  ctx.fillStyle = "rgb(0 200 50 / 70%)";
  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.arc(0, 0, 20, 0 - 1, 1);
  ctx.lineTo(0, 0);
  ctx.fill();
  ctx.stroke();

  ctx.beginPath();
  ctx.strokeStyle = "rgb(200 0 0 / 70%)";
  ctx.moveTo(0, 0);
  ctx.lineTo(60, 0);
  ctx.moveTo(0, -60);
  ctx.lineTo(0, 60);
  ctx.stroke();
  ctx.closePath();
  ctx.restore();
}

function transformFromSensorSpace(sensor, x, y) {
  let sensorY = y; //sensor.y - Math.cos(sensor.rot) * y - Math.sin(sensor.rot) * x;
  let sensorX = x; //sensor.x - Math.sin(sensor.rot) * x + Math.cos(sensor.rot) * y;
  ctx.beginPath();
  ctx.strokeStyle = "rgb(0 0 0 / 70%)";
  ctx.fillStyle = "rgb(0 50 200 / 70%)";
  ctx.arc(sensorX, sensorY, 10, 0, Math.PI * 2);
  ctx.fill();
  ctx.stroke();
  return { x: sensorX, y: sensorY };
}
function getMousePos(event) {
  return {
    x: Math.round(
      (event.clientX / c.clientWidth) * 429 - c.clientLeft - c.offsetLeft
    ),
    y: Math.round(
      (event.clientY / c.clientHeight) * 452 - c.clientTop - c.offsetTop
    ),
  };
}

function drawSpace(space, id, occupied) {
  ctx.beginPath();
  if (occupied) ctx.fillStyle = "rgb(200 50 0 / 70%)";
  else ctx.fillStyle = "rgb(100 50 0 / 70%)";
  if (id == editMode) ctx.fillStyle = "rgb(100 100 100 / 70%)";
  if (id == 1) ctx.fillStyle = "transparent";
  ctx.fillRect(space.x, space.y, space.width, space.height);
  ctx.strokeText(id, space.x + space.width / 2, space.y + space.height / 2);
  ctx.fill();
  ctx.stroke();
}
function redrawAll() {
  ctx.drawImage(img, 0, 0);
  drawSensor(sensor);
  let idx = 0;
  spaces.forEach((element) => {
    const occupied =
      element.x < person.x &&
      element.x + element.width > person.x &&
      element.y < person.y &&
      element.y + element.height > person.y;
    drawSpace(element, ++idx, occupied);
  });
  transformFromSensorSpace(sensor, person.x, person.y);
}
function updateConfig() {
  const json = JSON.stringify({ spaces: spaces, sensor: sensor });
  console.log(json);
  fetch("/update", {
    method: "POST",
    body: json,
    headers: {
      "Content-type": "application/json; charset=UTF-8",
    },
  });
}
