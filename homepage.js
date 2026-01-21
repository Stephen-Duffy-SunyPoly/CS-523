function preload(){
    //load the logo image
    logoImage = loadImage('logo.png');
}

let logoImage;
let numberOfSlices = 75
let logoSlices = []
let sliceWidth
let sliceOffset = []

function setup(){
    document.body.insertBefore(document.body.lastChild,document.body.firstChild);//make the canvas the first thing in the doc
    createCanvas(windowWidth-18, windowHeight);
    logoImage.resize(0,960 * (height/957))//resize the image to fit the screen size
    sliceWidth = int(logoImage.width/numberOfSlices)
    numberOfSlices = int(logoImage.width/sliceWidth)
    for(let i = 0; i < numberOfSlices; i++){
        let slice = createImage(sliceWidth,logoImage.height)
        slice.blend(logoImage,i*sliceWidth,0,sliceWidth,logoImage.height,0,0,slice.width,slice.height,REPLACE);
        logoSlices.push(slice);
        sliceOffset.push(500+10*i)
    }
    let lastSLice = createImage(sliceWidth,logoImage.height)
    lastSLice.blend(logoImage,numberOfSlices*sliceWidth,0,logoImage.width-numberOfSlices*sliceWidth,logoImage.height,0,0,logoImage.width-numberOfSlices*sliceWidth,lastSLice.height,REPLACE);
    logoSlices.push(lastSLice);
    sliceOffset.push(500+10*numberOfSlices)

}

function draw(){
    background(0);
    let topX = width/2-logoImage.width/2
    let topY = height/2-logoImage.height/2
    // image(logoImage,topX,topY);
    for(let i = 0; i < logoSlices.length; i++){
        let offset = sliceOffset[i]*(i%2===0?-1:1)
        image(logoSlices[i],topX+i*sliceWidth,topY+offset)
        if(sliceOffset[i]>0){
            sliceOffset[i]-=5
        }
    }
}

function mouseMoved() {
    // detect where the mouse is
    let topX = width/2-logoImage.width/2
    if(mouseX >= topX && mouseX <= topX+logoImage.width && mouseY < height){
        let xpos = mouseX - topX;
        let index = Math.floor(xpos/sliceWidth);
        sliceOffset[index]+=70
    }
}