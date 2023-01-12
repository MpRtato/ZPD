#include <Arduino.h>

//bibliotekas
#include <TEA5767.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Array.h>

//potenciometrs
int pot=A0; //potenciometra analoga pin

//poga
int poga=2; //pogas digitalais pin
int turlaiks=3; //pogas turesanas laiks (sekundes) lai mainitu rezimus

//fm recivers
TEA5767 radio = TEA5767(); //SDA analog pin A4; SCL pin A5
int minstiprums=12; //minimalais pielaujamais frekvences stiprums lai identificetu radio signalu;     stiprums: no 0 - 15

//lcd
LiquidCrystal_I2C lcd(0x27, 16, 2); //SDA analog pin A4; SCL pin A5

//funkcijas
void frekvencesmaina(); //funkcija kura nodrosina potenciometra darbibu mainot frekvenci
void signalurezims(); //funkcija kura inicialize staciju meklesanu

//datu kopa
const int maxelementi = 99;
Array<float,maxelementi> signali; //datu kopa, kura uzglaba kriterijiem atbilstosas frekvences

//mainigo definesana
float potval=0; //atgriesta potenciometra vertiba
bool pogaval=0; //atgriesta pogas vertiba
float frekv=87.50; //no 87.50MHz - 108.00MHz
float iepfrekv=0; //iepriekseja frekvcences
int frekvstiprums=0; //no 0 - 15
int iepfrekvstiprums=0; //ieprieksejais frekvcences stiprums
bool status=0; //ieslegts/izslegts MUTE rezims
bool pcikls=1; //norada vai ir pirmais cikls
bool stmaina=0; //norada ierices statusa mainu (no izslegta stavokla uz ieslegtu)
int signalusk=0; //atrasto radio signalu skaits
bool pmute=0; //norada vai ir pirmais MUTE (skanas izsklegsanas) rezima cikls
float sfrekvence=0.00; //stipraka frekvence
int sfrekvstiprums=0; //stiprakais frekvences stiprums
int lielfrekv=0; //frekvence bez decimal vertibas
int ieplielfrekv=0; //iepriekseja frekvence bez decimal vertibas
bool pfcikls=0; //norada vai ir pirmais pec rezimu mainas cikls


void setup() {
  Serial.begin(250000); //jabut 250000, savadak dati neattelojas pareizi Serial monitora
  Wire.begin(); //sekme I2C iericu darbibu

  //potenciometrs
  pinMode(pot,INPUT);

  //poga
  pinMode(poga,INPUT);

  //fm reciveris
  radio.setMuted(1); //izsledz/iesledz TEA4767 modula skanu

  //lcd
  lcd.init(); //inicialize LCD ekranu
  lcd.backlight(); //iesledz LCD ekrana aizmugures gaismu, var teikt ka pasu ekranu
  lcd.clear(); //nodzes visu informaciju, kas ir uz LCD ekrana

  lcd.setCursor(0,0); //novieto rakstisanas vietas sakumu (liniku un kollonu) uz LCD ekrana
  Serial.println("IZSLEGTS"); //parada noteiktu informaciju Serial monitora
  lcd.print("----IZSLEGTS----"); //attelo noteiktu informaciju LCD ekrana

}


void loop() { //galvena dala, kas nodrosina recivera inicializesanu, rezimu parsleksanu, pirmo rezimu un MUTE funkcionalitati
  pogaval=digitalRead(poga); //nolasa pogas vertibas
  if (pcikls==0){
    if (pogaval==1){ //nosaka vai poga ir nospiesta, ja ir tad bloke citas darbibas, sak nospiesta stavokla laika atskaiti, sasniedzot noteikto laiku parsledz reciveri uz otro rezimu
      Serial.println("MUTE");
      radio.setMuted(1);

      int i=0;
      while(pogaval==1){
        pogaval=digitalRead(poga);

        i++;
        delay(10);

        if(i==turlaiks*100){
          lcd.clear();

          signalurezims();

        }

      }

      status=0;
      pmute=1;

    }

  }
  
  while(status==0){ //parbauda vai ierices skana ir ieslegta, veic stavoklim atbilstosas darbibas
    if(pfcikls==1){
      status=1;

    }

    pogaval=digitalRead(poga);

    if(pcikls==0){
      frekvencesmaina(); //pieprasijums pec ar potenciometru iegutas frekvences

      if(frekv!=iepfrekv && frekvstiprums!=iepfrekvstiprums || pmute==1){ //optimizacija, kas mazina LCD ekrana datu bezjedzigu atjaunosanu, samazina signala traucejumus
      //ar potenciometru iegutas frekvences un tas stipruma attelosana LCD ekrana
      lcd.clear();

      lcd.setCursor(0,0);
      lcd.print("Frekvence:");
      lcd.setCursor(10,0);
      lcd.print(frekv);

      lcd.setCursor(0,1);
      lcd.print("Stiprums:");
      lcd.setCursor(9,1);
      lcd.print(frekvstiprums);

      if(pfcikls==0){
        lcd.setCursor(12,1);
        lcd.print("MUTE");

      }

      }

    }

    pmute=0;
    iepfrekv=frekv;
    iepfrekvstiprums=frekvstiprums;

    delay(100);
    
    if(pogaval==1){
      //recivera inicializacija
      if(pcikls==1){
        lcd.clear();

        lcd.setCursor(0,0);
        lcd.print("INICIALIZACIJA:");

        while(pogaval==1){
          pogaval=digitalRead(poga);

        }

        //radio signalu meklesana
        Serial.println("INICIALIZEJAS");
        //frekvencu stiprumu parbaude un stiprako atlasisana
        frekv=87.50;
        frekvstiprums=0;
        int i=0;

        while(frekv<=108.00){
          radio.setFrequency(frekv); //iestata frekvenci TEA4767 moduli
          delay(10);
          frekvstiprums=radio.getSignalLevel(); //iegust frekvences signala stiprumu no TEA4767 modula

          lielfrekv=int(frekv);
          if(lielfrekv==ieplielfrekv){
            if(frekvstiprums>=minstiprums){
              if(frekvstiprums>sfrekvstiprums){
                  sfrekvence=frekv;

                }

              }
            
          }
            
          else{
            if(sfrekvence!=0){
              Serial.println(sfrekvence); //stiprakas frekvences, no tas grupas, attelosana Serial monitora
              signali.push_back(sfrekvence); //stiprakas frekvences, no tas grupas, ievietosana atbilstoso frekvencu datu masiva

              if(i>=16){ //inicializacijas notiksanas un tas statusa attelojums uz LCD ekrana
                i=0;
                lcd.clear();

                lcd.setCursor(0,0);
                lcd.print("INICIALIZACIJA:");

                lcd.setCursor(0,1);
                lcd.print((char)255);
                
                
              }

              else{
                lcd.setCursor(i,1);
                lcd.print((char)255);

              }

              i++;

            }

          sfrekvence=0;

          }

          ieplielfrekv=lielfrekv;
          frekv=frekv+0.01;

        }

        signalusk=signali.size(); //datu masiva ar atbilsotsajam frekvencem izmera definesana
        //atlasito signalu skaita attelosana Serial monitora
        Serial.print("Signalu skaits: ");
        Serial.println(signalusk);

        radio.setMuted(0);

      }
      
      //norada ka recivers ir nav apklusinats, veic stavoklim atbilstosas darbibas
      status=1;
      stmaina=1;
      Serial.println("UNMUTE");
      radio.setMuted(0);

      int i=0;
      while(pogaval==1){
        pogaval=digitalRead(poga);

        i++;
        delay(10);

        if(i==turlaiks*100){
          lcd.clear();

          signalurezims();

        }

      }

      pcikls=0;
      break;

    }

  }

  pfcikls=0;

  frekvencesmaina();
  if(stmaina==1){
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Frekvence:");
    lcd.setCursor(10,0);
    lcd.print(frekv);

    lcd.setCursor(0,1);
    lcd.print("Stiprums:");
    lcd.setCursor(9,1);
    lcd.print(frekvstiprums);

    stmaina=0;

  }

  else if(frekv!=iepfrekv && frekvstiprums!=iepfrekvstiprums){
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Frekvence:");
    lcd.setCursor(10,0);
    lcd.print(frekv);

    lcd.setCursor(0,1);
    lcd.print("Stiprums:");
    lcd.setCursor(9,1);
    lcd.print(frekvstiprums);

  }

  iepfrekv=frekv;
  iepfrekvstiprums=frekvstiprums;

  delay(100);
  
}


void frekvencesmaina(){ //funkcijas, kas nodrosina frekvences mainu ar potenciometru
  potval=analogRead(pot); //nolasa potenciometra vertibas
  potval= map(potval, 2, 1014, 8750, 10800);
  frekv=potval/100;
  radio.setFrequency(frekv);
  delay(10);

  frekvstiprums=radio.getSignalLevel();

  //ar potenciometru iegutas frekvences un tas stipruma attelosana Serial monitora
  Serial.print("Frekvence: ");
  Serial.println(frekv);

  Serial.print("Stiprums ");
  Serial.println(frekvstiprums);

}


void signalurezims(){ //funkcija, kas kalpo, ka otrais recivera rezims
    radio.setMuted(0);
    int i=0;

    //pirmas kriterijiem atbilstosas frekvences/signala attelosana LCD ekrana
    Serial.println(signali[i]); //attelo Serial monitora paslaik izveleto frekvenci
    
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Frekvence:");
    lcd.setCursor(10,0);
    lcd.print(signali[i]);

    lcd.setCursor(0,1);
    lcd.print("Signals:");
    lcd.setCursor(8,1);
    lcd.print(i+1);
    lcd.setCursor(9,1);
    lcd.print("/");
    lcd.setCursor(10,1);
    lcd.print(signalusk);

    i++;
    while(true){ //nelauj funkcijai beigties, sevi ietver otro recivera dalu, sekme parsleksanos starp atbilstosajam frekvencem
      pogaval=digitalRead(poga);

      if(pogaval==1){
        int j=0;
        while(pogaval==1){
          pogaval=digitalRead(poga);

          j++;
          delay(10);

          if(j==turlaiks*100){
            lcd.clear();
            
            pfcikls=1;
            return; //apstadina funkciju

          }

        }
    
        Serial.println(signali[i]);
        radio.setFrequency(signali[i]);

        //parejo kriterijiem atbilstoso frekvencu/signalu attelosana LCD ekrana
        lcd.clear();

        lcd.setCursor(0,0);
        lcd.print("Frekvence:");
        lcd.setCursor(10,0);
        lcd.print(signali[i]);

        lcd.setCursor(0,1);
        lcd.print("Signals:");

        if(i+1<10){
        lcd.setCursor(8,1);
        lcd.print(i+1);
        lcd.setCursor(9,1);
        lcd.print("/");
        lcd.setCursor(10,1);
        lcd.print(signalusk);

        }

        else{
        lcd.setCursor(8,1);
        lcd.print(i+1);
        lcd.setCursor(10,1);
        lcd.print("/");
        lcd.setCursor(11,1);
        lcd.print(signalusk);

        }


        i++;
        if(i>signalusk-1){
          i=0;

        }

      }

    }

  }
