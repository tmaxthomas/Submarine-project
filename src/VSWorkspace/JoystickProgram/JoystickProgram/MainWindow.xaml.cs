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
        private String serialPortID = "";
        

        private DispatcherTimer timer;

        // Initialize DirectInput
        DirectInput directInput = new DirectInput();
        // Find a Joystick Guid
        Guid joystickGuid = Guid.Empty;
        Joystick joystick;

        // Setpoint Data variables
        bool headlightSwitch = true;
		bool isEStop = false;

        SByte driveSetpoint = 0;
        SByte rudderSetpoint = 0;
        SByte aftDiveSetpoint = 0;
        SByte foreDiveSetpoint = 0;
        Byte headLightSetpoint = 0;
        UInt16 spoolSetpoint = 0;
        UInt16 ballastSetpoint = 0;

        int serialTransmitCounter = 0;
        int serialReceiveDelayCounter = 0;

        //Submarine Running Variables
        SByte rudderPosition = 0;
        SByte aftDivePosition = 0;
        SByte foreDivePosition = 0;
        UInt16 spoolPosition = 0;
        UInt16 ballastPosition = 0;
        Byte motorTemp = 0;
        Byte waterSense = 0;
        Byte batteryVoltage = 0;

		Byte[] tempData = new byte[50];

		public MainWindow(){
            InitializeComponent();
            
            String[] ports = SerialPort.GetPortNames();
            for(int i = 0; i < ports.Length; i++){
                SerialComboBox.Items.Add(ports.ElementAt(i));
            }

            
            Loaded += new RoutedEventHandler(Window1_Loaded);
            
            Thread thread1 = new Thread(runningThread);
            thread1.Start();

        }
        
        void Window1_Loaded(object sender, RoutedEventArgs e){

            timer = new DispatcherTimer();
            timer.Tick += new EventHandler(dispatcherTimer_Tick);
            timer.Interval = new TimeSpan(0, 0, 0, 0, 250);
            timer.Start();
        }
        
        private void dispatcherTimer_Tick(object sender, EventArgs e) {
            updateFrame();
        }

        public void updateFrame() {
            SpoolFeedbackSlider.Value = spoolPosition;
            BallastFeedbackSlider.Value = ballastPosition;
            AftDiveSlider.Value = aftDivePosition;
            ForeDiveSlider.Value = foreDivePosition;
            RudderFeedbackSlider.Value = rudderPosition;
            WaterSensorSlider.Value = waterSense;
            BatteryVoltageSlider.Value = batteryVoltage;
            MotorTempSlider.Value = motorTemp;

            ThrottleSetpointSlider.Value = driveSetpoint * -1;
            DiveAngleSlider.Value = foreDiveSetpoint;
            RudderAngleSlider.Value = rudderSetpoint * -1;
            BallastSetpointSlider.Value = ballastSetpoint;
            SpoolSetpointSlider.Value = spoolSetpoint;
            HeadlightsCheckbox.IsChecked = !headlightSwitch;

			if (isEStop){
				EStopLabel.Visibility = Visibility.Visible;
			}

        }

        public void runningThread(){

            while (!isEnabled) ;

            foreach (var deviceInstance in directInput.GetDevices(DeviceType.FirstPerson,
                            DeviceEnumerationFlags.AllDevices))
                joystickGuid = deviceInstance.InstanceGuid;

            // If Gamepad not found, look for a Joystick
            if (joystickGuid == Guid.Empty)
                foreach (var deviceInstance in directInput.GetDevices(DeviceType.Joystick,
                        DeviceEnumerationFlags.AllDevices))
                    joystickGuid = deviceInstance.InstanceGuid;

            // If Joystick not found, throws an error
            if (joystickGuid == Guid.Empty){
                return;
            }

            // Instantiate the joystick
            joystick = new Joystick(directInput, joystickGuid);

            // Set BufferSize in order to use buffered data.
            joystick.Properties.BufferSize = 128;

            // Acquire the joystick
            joystick.Acquire();

            SerialPort SerialPort1 = new SerialPort();
            SerialPort1.PortName = serialPortID;
            SerialPort1.BaudRate = 9600;
            try{
                SerialPort1.Open();
                isEnabled = true;
            }
            catch{
                isEnabled = false;
            }

            while (true){
                if (isEnabled){
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
                    foreach (var state in datas){

                        //X Change - Rudder
                        if (state.RawOffset == 0){
                            rudderSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                        }

                        //Y Change - Dive Angle
                        else if (state.RawOffset == 4){
                            foreDiveSetpoint = (SByte)(((state.Value - 32768) * -1) / 500);
                            aftDiveSetpoint = (SByte)(-1 * foreDiveSetpoint);

                        }

                        //Z Change - Throttle
                        else if (state.RawOffset == 8){
                            driveSetpoint = (SByte)((state.Value - 32768) / 500);
                        }

                        //Extra Axis - Spool (light up knob on the side of throttle)
                        else if (state.RawOffset == 12){
                            spoolSetpoint = (UInt16)(state.Value / 94);
                        }

                        //Extra Axis - Ballast (light up knob on top of throttle)
                        else if (state.RawOffset == 16){
                            ballastSetpoint = (UInt16)(state.Value / 180);
                        }

                        //Button - headlights
                        else if (state.RawOffset == 48){
                            if (state.Value == 128){
                                if (headlightSwitch){
                                    headLightSetpoint = 100;
                                    headlightSwitch = false;
                                }
                                else{
                                    headLightSetpoint = 0;
                                    headlightSwitch = true;
                                }
                            }
                        }

						else if (state.RawOffset == 49){
							isEStop = true;
						}

                    }

                    //Serial Data Transmit
                    if (serialTransmitCounter == 5){
                        Byte[] stationPacket = new byte[10];

                        stationPacket[0] = (Byte)driveSetpoint;
                        stationPacket[1] = (Byte)rudderSetpoint;
                        stationPacket[2] = (Byte)aftDiveSetpoint;
                        stationPacket[3] = (Byte)foreDiveSetpoint;
                        stationPacket[4] = headLightSetpoint;
                        stationPacket[5] = (Byte)(spoolSetpoint >> 8);
                        stationPacket[6] = (Byte)(spoolSetpoint);
                        stationPacket[7] = (Byte)(ballastSetpoint >> 8);
                        stationPacket[8] = (Byte)(ballastSetpoint);
						if (isEStop){
							stationPacket[9] = (Byte)30;
						}
						else{
							stationPacket[9] = (Byte)10;
						}

                        SerialPort1.Write(stationPacket, 0, stationPacket.Length);
                        serialTransmitCounter = 0;
                    }

                    serialTransmitCounter++;
                    
                    if (SerialPort1.BytesToRead != 0){
                        serialReceiveDelayCounter++;
                    }
                    
                    if (serialReceiveDelayCounter == 2){


                        if (SerialPort1.BytesToRead == 11){

                            Byte[] subPacket = new byte[11];

                            for(int i = 0; i < subPacket.Length; i++) {
                                subPacket[i] = (Byte)SerialPort1.ReadByte();
                            }

                            rudderPosition = (SByte)subPacket[0];
                            aftDivePosition = (SByte)subPacket[1];
                            foreDivePosition = (SByte)subPacket[2];
                            spoolPosition = (UInt16)subPacket[3];
                            spoolPosition = (UInt16)(spoolPosition << 8);
                            spoolPosition = (UInt16)(spoolPosition | (UInt16)subPacket[4]);
                            ballastPosition = (UInt16)subPacket[5];
                            ballastPosition = (UInt16)(ballastPosition << 8);
                            ballastPosition = (UInt16)(ballastPosition | (UInt16)subPacket[6]);
                            motorTemp = subPacket[7];
                            waterSense = subPacket[8];
                            batteryVoltage = subPacket[9];
                            //need subPacketCHeck implementation

                        }
                        else if((SerialPort1.BytesToRead % 11) == 0) {
                            int discardBytes = SerialPort1.BytesToRead - 11;
                            for(int i = 0; i < discardBytes; i++) {
                                SerialPort1.ReadByte();
                            }

                            Byte[] subPacket = new byte[11];

                            for (int i = 0; i < subPacket.Length; i++) {
                                subPacket[i] = (Byte)SerialPort1.ReadByte();
                            }

                            rudderPosition = (SByte)subPacket[0];
                            aftDivePosition = (SByte)subPacket[1];
                            foreDivePosition = (SByte)subPacket[2];
                            spoolPosition = (UInt16)subPacket[3];
                            spoolPosition = (UInt16)(spoolPosition << 8);
                            spoolPosition = (UInt16)(spoolPosition | (UInt16)subPacket[4]);
                            ballastPosition = (UInt16)subPacket[5];
                            ballastPosition = (UInt16)(ballastPosition << 8);
                            ballastPosition = (UInt16)(ballastPosition | (UInt16)subPacket[6]);
                            motorTemp = subPacket[7];
                            waterSense = subPacket[8];
                            batteryVoltage = subPacket[9];

                        }
                        else {
                            SerialPort1.DiscardInBuffer();
                        }

                         
                        serialReceiveDelayCounter = 0;

                    }
					
                    
                }

                Thread.Sleep(10);
            }
            
        }
        private void SerialButton_Click(object sender, RoutedEventArgs e){
            serialPortID = SerialComboBox.Text;
            isEnabled = true;

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
