using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using SharpDX.DirectInput;
using System.IO.Ports;
using System.Windows.Threading;
using System.Threading;

namespace JoystickProgram{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window{


        private bool isEnabled = false;
        SerialPort SerialPort1 = new SerialPort();

        //private DispatcherTimer timer;

        // Initialize DirectInput
        DirectInput directInput = new DirectInput();
        // Find a Joystick Guid
        Guid joystickGuid = Guid.Empty;
        Joystick joystick;

        // Setpoint Data variables
        bool headlightSwitch = true;

        SByte driveSetpoint = 0;
        SByte rudderSetpoint = 0;
        SByte aftDiveSetpoint = 0;
        SByte foreDiveSetpoint = 0;
        Byte headLightSetpoint = 0;
        UInt16 spoolSetpoint = 0;
        UInt16 ballastSetpoint = 0;

        int serialTransmitCounter = 0;
        int serialReceiveDelayCounter = 0;


        public MainWindow(){
            InitializeComponent();
            
            String[] ports = SerialPort.GetPortNames();
            for(int i = 0; i < ports.Length; i++){
                SerialComboBox.Items.Add(ports.ElementAt(i));
            }

            /*
            Loaded += new RoutedEventHandler(Window1_Loaded);
            */
            Thread thread1 = new Thread(runningThread);
            thread1.Start();

        }
        /*
        void Window1_Loaded(object sender, RoutedEventArgs e){

            timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromMilliseconds(10);
            timer.Tick += timer1_Tick;
            timer.Start();

        }
        */
        public void runningThread(){

            foreach (var deviceInstance in directInput.GetDevices(DeviceType.FirstPerson,
                            DeviceEnumerationFlags.AllDevices))
                joystickGuid = deviceInstance.InstanceGuid;

            // If Gamepad not found, look for a Joystick
            if (joystickGuid == Guid.Empty)
                foreach (var deviceInstance in directInput.GetDevices(DeviceType.Joystick,
                        DeviceEnumerationFlags.AllDevices))
                    joystickGuid = deviceInstance.InstanceGuid;

            // If Joystick not found, throws an error
            if (joystickGuid == Guid.Empty)
            {
                NoJoystickFoundText.IsEnabled = true;
                return;
            }

            // Instantiate the joystick
            joystick = new Joystick(directInput, joystickGuid);

            // Set BufferSize in order to use buffered data.
            joystick.Properties.BufferSize = 128;

            // Acquire the joystick
            joystick.Acquire();

            while (true)
            {
                if (isEnabled)
                {
                    joystick.Poll();
                    var datas = joystick.GetBufferedData();
                    /*
                     *  RawOffset - Button or Axis
                     *      X: 0
                     *      Y: 4
                     *      Z: 8
                     *      ButtonTrigger: 48
                     *  Value - the value of the axis or button (axis are uint16_t, buttons are 128 when pressed, 0 when released)
                     */
                    foreach (var state in datas)
                    {

                        //X Change - Rudder
                        if (state.RawOffset == 0)
                        {
                            rudderSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                        }

                        //Y Change - Dive Angle
                        if (state.RawOffset == 4)
                        {
                            aftDiveSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                            foreDiveSetpoint = (SByte)(-1 * aftDiveSetpoint);

                        }

                        //Z Change - Throttle
                        if (state.RawOffset == 8)
                        {
                            driveSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                        }

                        //Extra Axis - Spool (light up knob on the side of throttle)
                        if (state.RawOffset == 12)
                        {
                            spoolSetpoint = (UInt16)(state.Value / 94);
                        }

                        //Extra Axis - Ballast (light up knob on top of throttle)
                        if (state.RawOffset == 16)
                        {
                            ballastSetpoint = (UInt16)(state.Value / 180);
                        }

                        //Button - headlights
                        if (state.RawOffset == 48)
                        {
                            if (state.Value == 128)
                            {
                                if (headlightSwitch)
                                {
                                    headLightSetpoint = 100;
                                    headlightSwitch = false;
                                }
                                else
                                {
                                    headLightSetpoint = 0;
                                    headlightSwitch = true;
                                }
                            }
                        }

                    }

                    //Serial Data Transmit
                    if (serialTransmitCounter == 5)
                    {
                        Byte[] stationPacket = new byte[9];

                        stationPacket[0] = (Byte)driveSetpoint;
                        stationPacket[1] = (Byte)rudderSetpoint;
                        stationPacket[2] = (Byte)aftDiveSetpoint;
                        stationPacket[3] = (Byte)foreDiveSetpoint;
                        stationPacket[4] = headLightSetpoint;
                        stationPacket[5] = (Byte)(spoolSetpoint >> 8);
                        stationPacket[6] = (Byte)(spoolSetpoint);
                        stationPacket[7] = (Byte)(ballastSetpoint >> 8);
                        stationPacket[8] = (Byte)(ballastSetpoint);

                        SerialPort1.Write(stationPacket, 0, stationPacket.Length);
                        // Console.WriteLine("test");
                        serialTransmitCounter = 0;
                    }

                    serialTransmitCounter++;

                    if (SerialPort1.BytesToRead != 0)
                    {
                        serialReceiveDelayCounter++;
                    }
                    if (serialReceiveDelayCounter == 2)
                    {

                        if (SerialPort1.BytesToRead == 9)
                        {
                            for (int i = 0; i < 9; i++)
                            {
                                Console.Write(SerialPort1.ReadByte());
                                Console.Write(" ");
                            }
                            Console.WriteLine(" ");
                        }

                        serialReceiveDelayCounter = 0;
                    }
                }

                Thread.Sleep(10);
            }
            
        }
        /*
        private void timer1_Tick(object sender, EventArgs e){
            
            if (isEnabled)
            {
                joystick.Poll();
                var datas = joystick.GetBufferedData();
                /*
                 *  RawOffset - Button or Axis
                 *      X: 0
                 *      Y: 4
                 *      Z: 8
                 *      ButtonTrigger: 48
                 *  Value - the value of the axis or button (axis are uint16_t, buttons are 128 when pressed, 0 when released)
                 *
                foreach (var state in datas)
                {

                    //X Change - Rudder
                    if (state.RawOffset == 0)
                    {
                        rudderSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                    }

                    //Y Change - Dive Angle
                    if (state.RawOffset == 4)
                    {
                        aftDiveSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                        foreDiveSetpoint = (SByte)(-1 * aftDiveSetpoint);

                    }

                    //Z Change - Throttle
                    if (state.RawOffset == 8)
                    {
                        driveSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                    }

                    //Extra Axis - Spool (light up knob on the side of throttle)
                    if (state.RawOffset == 12)
                    {
                        spoolSetpoint = (UInt16)(state.Value / 94);
                    }

                    //Extra Axis - Ballast (light up knob on top of throttle)
                    if (state.RawOffset == 16)
                    {
                        ballastSetpoint = (UInt16)(state.Value / 180);
                    }

                    //Button - headlights
                    if (state.RawOffset == 48)
                    {
                        if (state.Value == 128)
                        {
                            if (headlightSwitch)
                            {
                                headLightSetpoint = 100;
                                headlightSwitch = false;
                            }
                            else
                            {
                                headLightSetpoint = 0;
                                headlightSwitch = true;
                            }
                        }
                    }

                }

                //Serial Data Transmit
                if (serialTransmitCounter == 5)
                {
                    Byte[] stationPacket = new byte[9];

                    stationPacket[0] = (Byte)driveSetpoint;
                    stationPacket[1] = (Byte)rudderSetpoint;
                    stationPacket[2] = (Byte)aftDiveSetpoint;
                    stationPacket[3] = (Byte)foreDiveSetpoint;
                    stationPacket[4] = headLightSetpoint;
                    stationPacket[5] = (Byte)(spoolSetpoint >> 8);
                    stationPacket[6] = (Byte)(spoolSetpoint);
                    stationPacket[7] = (Byte)(ballastSetpoint >> 8);
                    stationPacket[8] = (Byte)(ballastSetpoint);

                    SerialPort1.Write(stationPacket, 0, stationPacket.Length);
                    // Console.WriteLine("test");
                    serialTransmitCounter = 0;
                }

                serialTransmitCounter++;

                if (SerialPort1.BytesToRead != 0)
                {
                    serialReceiveDelayCounter++;
                }
                if (serialReceiveDelayCounter == 2)
                {

                    if (SerialPort1.BytesToRead == 9)
                    {
                        for (int i = 0; i < 9; i++)
                        {
                            Console.Write(SerialPort1.ReadByte());
                            Console.Write(" ");
                        }
                        Console.WriteLine(" ");
                    }

                    serialReceiveDelayCounter = 0;
                }
            }

            

            }
            */
            private void SerialButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {

                SerialPort1.PortName = SerialComboBox.Text;
                SerialPort1.BaudRate = 9600;
                SerialPort1.Open();

                isEnabled = true;

                /*
                
                byte[] myByteArray = { 2, 4 };
                SerialPort1.Write(myByteArray, 0, 2);
                
                while (SerialPort1.BytesToRead != 2) { }
                Console.WriteLine(SerialPort1.BytesToRead);
                Console.WriteLine(SerialPort1.ReadByte());
                Console.WriteLine(SerialPort1.ReadByte());

                //Console.Write(SerialPort1.ReadByte());
                testText.IsEnabled = true;
                /*byte[] myReceivedData = null;

                SerialPort1.Read(myReceivedData, 0, 2);
                
                String myText = myReceivedData[0].ToString();
                myText += myReceivedData[1].ToString();

                TextBox1.Text = myText;
              //  isSerialEnabled = false;
              */
            }
            catch
            {
                isEnabled = false;
            }
            if (isEnabled)
            {
                SpoolFeedbackSlider.IsEnabled = true;
                SpoolSetpointSlider.IsEnabled = true;
                BallastFeedbackSlider.IsEnabled = true;
                BallastSetpointSlider.IsEnabled = true;
                DiveAngleSlider.IsEnabled = true;
                ForeDiveSlider.IsEnabled = true;
                AftDiveSlider.IsEnabled = true;
                RudderFeedbackSlider.IsEnabled = true;
                RudderAngleSlider.IsEnabled = true;
                WaterSensorSlider.IsEnabled = true;
                BatteryVoltageSlider.IsEnabled = true;
                MotorTempSlider.IsEnabled = true;
                ThrottleSetpointSlider.IsEnabled = true;
                HeadlightsCheckbox.IsEnabled = true;

                
                

                
                
            }
        }
    }
}
