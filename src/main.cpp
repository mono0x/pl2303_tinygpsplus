/* USB Host to PL2303-based USB GPS unit interface */
/* Navibee GM720 receiver - Sirf Star III */
/* Mikal Hart's TinyGPS library */
/* test_with_gps_device library example modified for PL2302 access */

/* USB support */
#include <usbhub.h>

/* CDC support */
#include <cdcacm.h>
#include <cdcprolific.h>

#include <TinyGPSPlus.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

/* This sample code demonstrates the normal use of a TinyGPS object.
    Modified to be used with USB Host Shield Library r2.0
    and USB Host Shield 2.0
*/

class PLAsyncOper : public CDCAsyncOper
{
public:
    uint8_t OnInit(ACM *pacm);
};

uint8_t PLAsyncOper::OnInit(ACM *pacm)
{
    uint8_t rcode;

    // Set DTR = 1
    rcode = pacm->SetControlLineState(1);

    if (rcode)
    {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }

    LINE_CODING lc;
    lc.dwDTERate = 115200; // default serial speed of GPS unit
    lc.bCharFormat = 0;
    lc.bParityType = 0;
    lc.bDataBits = 8;

    rcode = pacm->SetLineCoding(&lc);

    if (rcode)
    {
        ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);
    }

    return rcode;
}

USB Usb;
USBHub Hub(&Usb);
PLAsyncOper AsyncOper;
PL2303 Pl(&Usb, &AsyncOper);
TinyGPSPlus gps;

void gpsdump(TinyGPSPlus &gps);
void stats(TinyGPSPlus &gps);
bool feedgps();

void setup()
{

    Serial.begin(115200);
#if !defined(__MIPSEL__)
    while (!Serial)
        ; // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif

    Serial.print("Testing TinyGPS library v. ");
    Serial.println(TinyGPSPlus::libraryVersion());
    Serial.println("by Mikal Hart");
    Serial.println();
    Serial.print("Sizeof(gpsobject) = ");
    Serial.println(sizeof(TinyGPSPlus));
    Serial.println();
    /* USB Initialization */
    if (Usb.Init() == -1)
    {
        Serial.println("OSCOKIRQ failed to assert");
    }

    delay(200);
}

void loop()
{
    Usb.Task();

    if (Pl.isReady())
    {

        bool newdata = false;
        uint32_t start = (uint32_t)millis();

        // Every 5 seconds we print an update
        while ((int32_t)((uint32_t)millis() - start) < 5000)
        {
            if (feedgps())
            {
                newdata = true;
            }
        } // while (millis()...

        if (newdata)
        {
            Serial.println("Acquired Data");
            Serial.println("-------------");
            gpsdump(gps);
            Serial.println("-------------");
            Serial.println();
        } // if( newdata...
    } // if( Usb.getUsbTaskState() == USB_STATE_RUNNING...
}

void gpsdump(TinyGPSPlus &gps)
{
    Serial.print("Location: ");
    if (gps.location.isValid())
    {
        Serial.printf("%.6f,%.6f", gps.location.lat(), gps.location.lng());
    }
    else
    {
        Serial.print("INVALID");
    }
    Serial.println();

    Serial.print("Date/Time: ");
    if (gps.date.isValid())
    {
        Serial.printf("%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
    }
    else
    {
        Serial.print("INVALID");
    }

    Serial.print(" ");
    if (gps.time.isValid())
    {
        Serial.printf("%02d:%02d:%02d.%02d", gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
    }
    else
    {
        Serial.print("INVALID");
    }
    Serial.println();

    feedgps();

    Serial.print("Altitude: ");
    if (gps.altitude.isValid())
    {
        Serial.printf("%.2fm", gps.altitude.meters());
    }
    else
    {
        Serial.print("INVALID");
    }
    Serial.println();

    Serial.print("Course: ");
    if (gps.course.isValid())
    {
        Serial.printf("%.2fdeg", gps.course.deg());
    }
    else
    {
        Serial.print("INVALID");
    }
    Serial.println();

    Serial.print("Speed: ");
    if (gps.speed.isValid())
    {
        Serial.printf("%.2fkm/h", gps.speed.kmph());
    }
    else
    {
        Serial.print("INVALID");
    }
    Serial.println();

    Serial.print("Satellites: ");
    if (gps.satellites.isValid())
    {
        Serial.print(gps.satellites.value());
    }
    else
    {
        Serial.print("INVALID");
    }
    Serial.println();

    feedgps();

    Serial.print("HDOP: ");
    if (gps.hdop.isValid())
    {
        Serial.printf("%.2f", gps.hdop.hdop());
    }
    else
    {
        Serial.print("INVALID");
    }
    Serial.println();

    Serial.print("Chars: ");
    Serial.print(gps.charsProcessed());
    Serial.print("  Sentences: ");
    Serial.print(gps.sentencesWithFix());
    Serial.print("  Failed checksum: ");
    Serial.print(gps.failedChecksum());
    Serial.println();
}

bool feedgps()
{
    uint8_t rcode;
    uint8_t buf[64]; // serial buffer equals Max.packet size of bulk-IN endpoint
    uint16_t rcvd = 64;
    {
        /* reading the GPS */
        rcode = Pl.RcvData(&rcvd, buf);
        if (rcode && rcode != hrNAK)
            ErrorMessage<uint8_t>(PSTR("Ret"), rcode);
        rcode = false;
        if (rcvd)
        { // more than zero bytes received
            for (uint16_t i = 0; i < rcvd; i++)
            {
                // Serial.print((char)buf[i]);
                if (gps.encode((char)buf[i]))
                { // feed a character to gps object
                    rcode = true;
                } // if( gps.encode(buf[i]...
            } // for( uint16_t i=0; i < rcvd; i++...
            // Serial.println();
            // Serial.printf("rcode=%d\n", rcode);
        } // if( rcvd...
    }
    return (rcode);
}
