import controlP5.*;

ControlP5 cp5;
Textfield textField;

//win size
int winX = 512;
int winY = 384;

//butt
boolean buttonPressed = false;
int buttonX = 5, buttonY = 6, buttonW = 100, buttonH = buttonY + 10;

//sweech
boolean isRGB565 = false;  // Переменная для переключения форматов
int swX = 110, swY = 6, swW = 100, swH = swY + 10;

//all
boolean isAll = true;
int allX = 215, allY = 6, allW = 100, allH = allY + 10;

//files
String fullFileName = "Файл не выбран";
String folderPath = "Папка не выбрана";
String fileName = "";
String name = "";
File[] files;

//Array
ArrayList<String> head = new ArrayList<String>();
ArrayList<String> top = new ArrayList<String>();
ArrayList<String> lines = new ArrayList<String>();
String line = "", s = "    ";;
int pos, b3, b2, b1, b0, numSprites;

// Список поддерживаемых расширений
String[] supportedExtensions = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff", ".wbmp", ".JPG", ".JPEG", ".PNG", ".GIF", ".BMP", ".TIF", ".TIF", ".TIFF", ".WBMP"};

//Image
  PImage img = null;
  int imgW, imgH;
  
void setup() {
  size(512, 384);

  // Вычисляем координаты для центрирования окна
  int x = (displayWidth - width) / 2;
  int y = (displayHeight - height) / 2;
  
  // Устанавливаем позицию окна
  surface.setLocation(x, y);
  
  // Инициализируем библиотеку ControlP5
  cp5 = new ControlP5(this); 
}  

void draw() {
  background(240);
  
// Рисуем кнопку
  fill(buttonPressed ? color(150) : color(200));
  rect(buttonX, buttonY, buttonW, buttonH, 10);  
  fill(0);
  textAlign(CENTER, CENTER);
  text("Open", buttonX + buttonW / 2, buttonY + buttonH / 2);  
  
//Check box
  fill(200);
  rect(swX, swY, swW, swH, 10);
  fill(0);
  textAlign(CENTER, CENTER);
  text("Color: " + (isRGB565 ? "RGB565" : "RGB332"), swX + swW / 2, swY + swH / 2); 

//all Check box
  fill(200);
  rect(allX, allY, allW, allH, 10);
  fill(0);
  textAlign(CENTER, CENTER);
  text((isAll ? "All files" : "One file"), allX + allW / 2, allY + allH / 2); 
  
  if (img != null) image(img, 0, 25);  // Рисуем изображение немного ниже кнопки
}  

void mousePressed() {
//butt  
  if (mouseX > buttonX && mouseX < buttonX + buttonW &&
      mouseY > buttonY && mouseY < buttonY + buttonH) {
    buttonPressed = true;
    selectInput("Выберите файл:", "fileSelected");
  }
  
//sweech
  // Проверяем клик по кнопке
  if (mouseX > swX && mouseX < swX + swW &&
      mouseY > swY && mouseY < swY + swH) {
    isRGB565 = !isRGB565;  // Переключаем формат
    println("Выбран формат: " + (isRGB565 ? "RGB565" : "RGB332"));
  }
  
//all sweech
  // Проверяем клик по кнопке
  if (mouseX > allX && mouseX < allX + allW &&
      mouseY > allY && mouseY < allY + allH) {
    isAll = !isAll;  // Переключаем формат
    println("Выбран формат: " + (isAll ? "All Files" : "One file"));
  }  
}

void mouseReleased() {
  buttonPressed = false;
}

void clearAll(){
  //Обнуление
  line = "";
  lines.clear(); lines.add(s + "//Sprites data");
  head.clear(); head.add(s + "//Sprite positions");  
  top.clear();
  img = null;
  pos = 0;
  numSprites = 0;
}

boolean checkFile(String f){
  // Проверяем расширение файла
  for (String ext : supportedExtensions) {
     if (f.toLowerCase().endsWith(ext.toLowerCase())) return true;
  }  
  
  return false;
}

String returnFileName(String n){
  int dotIndex = n.lastIndexOf('.');
      
  if (dotIndex > 0) {                // если есть точка и она не в начале
    return n.substring(0, dotIndex); // "example"
  } else {
    return n;                        // файл без расширения
  }   
}

void reSizeWin(){
  int xx = imgW + 5;
  int yy = imgH + 25;
  if ((winX < xx) || (winY < yy)) {
    surface.setSize(xx, yy);
  } else {
    surface.setSize(winX, winY);
  }  
}

void makeTop(String s){
  line = "#pragma once\n"; top.add(line);
  line = "const uint8_t _" + s + "[] PROGMEM = {"; top.add(line); 
}

void makeHead(){
}

void makeLines(){
  //Image size
  b3 = (img.width >> 8) & 0xFF;
  b2 = img.width & 0xFF;
  b1 = (img.height >> 8) & 0xFF;
  b0 = img.height & 0xFF;
  line = s + "0x" + hex(b3, 2) + ", 0x" + hex(b2, 2) + 
           ", 0x" + hex(b1, 2) + ", 0x" + hex(b0, 2) + 
           ", //" + img.width + " x " + img.height +
           " pos:" + pos; lines.add(line);

  for (int y = 0; y < img.height; y++){
    line = s;
    for (int x = 0; x < img.width; x++){
      int c = img.get(x, y);
      int r = (int) red(c);
      int g = (int) green(c);
      int b = (int) blue(c);
      
      if (isRGB565){
        int col = getColor(r, g, b);
        b1 = (col >> 8) & 0xFF;
        b0 = col & 0xFF;
        line += "0x" + hex(b1, 2) + ", 0x" + hex(b0, 2) + ", ";
      } else {
        line += "0x" + hex(getColor(r, g, b), 2) + ", ";        
      } 
      if ((x == img.width - 1 && y == img.height - 1)) line += "\n";
    }  
    
    lines.add(line);
  }  

  b3 = (pos >> 24) & 0xFF;
  b2 = (pos >> 16) & 0xFF;
  b1 = (pos >> 8) & 0xFF;
  b0 = pos & 0xFF;
  line = s + "0x" + hex(b3, 2) + ", 0x" + hex(b2, 2) + ", 0x" + hex(b1, 2) + ", 0x" + hex(b0, 2) + ", //" + pos;
  head.add(line);
  
  pos += ((isRGB565) ? img.width * img.height << 1 : img.width * img.height); 
  pos += 4;
  numSprites++;
}

void makeFile(){  
  line = s + ((isRGB565) ? "0x10, " : "0x08, ") + "0x01, // Color bit, num of sprites\n"; top.add(line);
  top.add(s + "//Sprite positions"); 
  top.add(s + "0x00, 0x00, 0x00, 0x00, //0\n");
  
  top.add(s + "//Sprites data");
  b3 = (img.width >> 8) & 0xFF;
  b2 = img.width & 0xFF;
  b1 = (img.height >> 8) & 0xFF;
  b0 = img.height & 0xFF;
  line = s + "0x" + hex(b3, 2) + ", 0x" + hex(b2, 2) + ", 0x" + hex(b1, 2) + ", 0x" + hex(b0, 2) + ", //" + img.width + " x " + img.height; top.add(line);

  for (int y = 0; y < img.height; y++){
    line = s;
    for (int x = 0; x < img.width; x++){
      int c = img.get(x, y);
      int r = (int) red(c);
      int g = (int) green(c);
      int b = (int) blue(c);
      
      if (isRGB565){
      } else {
        line += "0x" + hex(getColor(r, g, b), 2);
        if (!(x == img.width - 1 && y == img.height - 1)) line += ", ";

      } 
    }  
    
    top.add(line);
  }    
  top.add("};");
}

void fileSelected(File file) {
  if (file != null) {
    fullFileName = file.getAbsolutePath(); // Полный путь к файлу
    folderPath = file.getParent();
    fileName = file.getName();
    
    if (checkFile(fileName)){ 
      clearAll();
      name = returnFileName(fileName);    
      makeTop(name);
    
      if (isAll){
        // Получаем список файлов в папке
        File folder = new File(folderPath);
        files = folder.listFiles(); 
      
        if (files != null) {
          for (File f : files) {
            fileName = f.getName();
            fullFileName = f.getAbsolutePath(); // Полный путь к файлу
          
            if (checkFile(fileName)){
              img = loadImage(fullFileName); noStroke();  // Отключаем контуры для изображения
              reSizeWin();
              println(f.getName()+ " " + returnFileName(f.getName()));
              makeLines();
            } 
          } 
          

        }  
      } else {
        img = loadImage(fullFileName); noStroke();  // Отключаем контуры для изображения
        reSizeWin();
        makeLines();
      } 
    
      //lines.remove(lines.size() - 1);
      line = "};"; lines.add(line);
      line = s + "0x" + ((isRGB565) ? "10, " : "08, ") + 
                 "0x" + hex(numSprites, 2) + ", //Color " + ((isRGB565) ? "16(565)" : "8(332)") + " bit, " + numSprites + " sprites\n";
      top.add(line);           
      top.addAll(head.subList(0, head.size()));
      top.add("");
      top.addAll(lines.subList(0, lines.size()));
      saveStrings(folderPath + "\\" + name + ".h", top.toArray(new String[0]));
    } 
  }
}  

int getColor(int r, int g, int b){
  return (isRGB565 ? ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)   //565
                   : ((r >> 5) << 5)  | ((g >> 5) << 2) | (b >> 6)); //332
}
