using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using ZedGraph;
using System.Security.Cryptography;


namespace DOAN1
{
    public partial class Form1 : Form
    {
        int tickStart = 0; //khai bao bien dung cho timer, chay cot thoi gian tinh bang ms
        string dataframe;
        private double setpoint; // Declare the setpoint variable as a field

        // Method or constructor where you initialize the setpoint variable
        private void InitializeSetpoint()
        {
            if (!double.TryParse(textBox_SetPoint.Text, out setpoint))
            {
                MessageBox.Show("Invalid setpoint value!");
                return;
            }
        }
        int startstopState;
        private int mode;
        public Form1()
        {
           
            InitializeComponent();
        }

       
        
        private delegate void SetDeleg(string text);

        private void Form1_Load(object sender, EventArgs e)
        {
            dataframe = "";
            startstopState = 0;
            string[] ports = SerialPort.GetPortNames();
            cboPort.Items.AddRange(ports);
            cboPort.SelectedIndex = 0;
            btnClose.Enabled = false;
            
            ComboBox_Mode.Items.Add("speed");
            ComboBox_Mode.Items.Add("position");
            ComboBox_Mode.SelectedIndex = 0; // Chọn chế độ mặc định

            // Đăng ký sự kiện SelectedIndexChanged để xử lý sự thay đổi chế độ
            ComboBox_Mode.SelectedIndexChanged += ComboBox_Mode_SelectedIndexChanged;

            //Vẽ đồ thị
            Control.CheckForIllegalCrossThreadCalls = false;
            GraphPane mypane = zedGraphControl1.GraphPane; //su dung loai graph pane
            // cac thong tin den do thi
            mypane.Title.Text = "Do thi dap ung";
            mypane.XAxis.Title.Text = "thoi gian";
            mypane.YAxis.Title.Text = "Toc do";
            //Định nghĩa list để vẽ đồ thị 
            RollingPointPairList list1 = new RollingPointPairList(1200);
            // Ở đây sử dụng list với 1200 điểm 
            RollingPointPairList list2 = new RollingPointPairList(1200);
            LineItem curve1 = mypane.AddCurve("đáp ứng", list1,Color.Red, SymbolType.None);
            LineItem curve2 = mypane.AddCurve("setpoint", list2, Color.Green, SymbolType.None);
            curve1.Line.Width = 3;
            timer1.Interval = 50;
            //Khoang cach la 50ms/1lan vẽ
            //định kiểu hiện thi cho trục thời gian (X)
            mypane.XAxis.Scale.Min = 0;
            mypane.XAxis.Scale.Max = 3000;
            mypane.YAxis.Scale.Min = 0;
            mypane.YAxis.Scale.Max = 400;
            mypane.XAxis.Scale.MinorStep = 1;
            mypane.XAxis.Scale.MajorStep = 5;
            mypane.XAxis.MajorGrid.IsVisible = true;
            mypane.YAxis.MajorGrid.IsVisible = true;
            mypane.Chart.Fill = new Fill(Color.White, Color.Beige, 45.0f);
                              
            //
            zedGraphControl1.AxisChange(); //gọi hàm xác định cỡ trục
            //khơi động timer về vị trí ban đầu
            tickStart = Environment.TickCount;

        }
        

        private void serialPort_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            string data = serialPort1.ReadLine();
            txtReceive.Text = data;
            //data = data.TrimEnd('\n');
            float val1 = float.Parse(data);
            //if(!String.IsNullOrEmpty(data)         
            
            if (zedGraphControl1.GraphPane.CurveList.Count <= 0)
                return;
            //Kiểm tra việc khởi tạo các đường curve
            //đua điểm về điểm xuất phát
            LineItem curve1 = zedGraphControl1.GraphPane.CurveList[0] as LineItem;
            if (curve1 == null)
                return;
            //list chứa các điểm 
            // get the pointpairlist
            IPointListEdit list1 = curve1.Points as IPointListEdit;

            if (list1 == null)
                return;
            // timer được tính bằng ms
            double time = (Environment.TickCount - tickStart) / 1000.0;
            //Hiện lên đồ thị
            list1.Add(time, val1);
            MasterPane masterPane = zedGraphControl1.MasterPane;
            GraphPane myPane = masterPane.PaneList[0];

            // Remove the existing setpoint line from the GraphObjList
            myPane.GraphObjList.RemoveAll(obj => obj is LineObj);

            // Draw the new setpoint line
            LineObj setpointLine = new LineObj(Color.Green, myPane.XAxis.Scale.Min, setpoint, myPane.XAxis.Scale.Max, setpoint);
            setpointLine.Line.Style = System.Drawing.Drawing2D.DashStyle.Solid;
            setpointLine.Line.Width = 1;
            myPane.GraphObjList.Add(setpointLine);

            //đoạn chương trình vẽ đồ thị
            Scale xscale = zedGraphControl1.GraphPane.XAxis.Scale;
            if (time > xscale.Max - xscale.MajorStep) ;
            {
                xscale.Max = time + xscale.MajorStep;
                
            }
            zedGraphControl1.AxisChange();
            zedGraphControl1.Invalidate();
          
        }
        private void timer1_Tick(object sender, EventArgs e)
        {
            
        }

        private void btnOpen_Click(object sender, EventArgs e)
        {
            btnOpen.Enabled = false;
            btnClose.Enabled = true;
            try
            {
                serialPort1.PortName = cboPort.Text;
                serialPort1.Open();
                label_connectstate.Text = "Serial is connected to " + cboPort.Text;
                label_connectstate.ForeColor = Color.Green;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Message", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            btnOpen.Enabled = true;
            btnClose.Enabled = false;
            try
            {
                serialPort1.Close();
                label_connectstate.Text = "Serial disconnected";
                label_connectstate.ForeColor = Color.Red;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Message", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (serialPort1.IsOpen)
                serialPort1.Close();
        }
        private void cboPort_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void Clearbtn_Click(object sender, EventArgs e)
        {
            if (zedGraphControl1.GraphPane.CurveList.Count > 0)
            {
                LineItem curve1 = zedGraphControl1.GraphPane.CurveList[0] as LineItem;
                if (curve1 != null)
                {
                    IPointListEdit list1 = curve1.Points as IPointListEdit;
                    if (list1 != null)
                    {
                        list1.Clear();
                        zedGraphControl1.AxisChange();
                        zedGraphControl1.Invalidate();

                        // Reset the timer and X axis scale
                        tickStart = Environment.TickCount;

                        GraphPane mypane = zedGraphControl1.GraphPane;
                        mypane.XAxis.Scale.Min = 0;
                        mypane.XAxis.Scale.Max = 30;
                        mypane.XAxis.Scale.MinorStep = 1;
                        mypane.XAxis.Scale.MajorStep = 5;
                    }
                }
            }
        }
        private string changeFormat(string value)
        {
            char[] charResult = { '0', '0', '0', '0', '0' };// tạo mảng 5 ptu kiểu char, gtri khởi tạo 0
            int index = 0;
            int len = value.Length;// chiều dài của value
            while (index < 5)
            {
                if (index < len)
                {
                    charResult[4 - index] = value[(len - 1) - index];
                    if (value[len - 1 - index] == '-')
                    {
                        charResult[4 - index] = '0';
                        charResult[0] = '-';
                    }
                }
                index++;
            }
            string result = new string(charResult);
            return result;
        }
        private void button_Update_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                dataframe = "";
                dataframe += Convert.ToString(startstopState);
                dataframe += changeFormat(textBox_SetPoint.Text);
                dataframe += changeFormat(textBox_Kp.Text);
                dataframe += changeFormat(textBox_Ki.Text);
                dataframe += changeFormat(textBox_Kd.Text);              
                setpoint = Convert.ToDouble(textBox_SetPoint.Text);
                dataframe += Convert.ToString(mode);

                serialPort1.Write(dataframe);
            }
        }

        private void button_Stop_Click(object sender, EventArgs e)
        {
            startstopState = 0;
            label_motorState.Text = "Motor is stopping";
            label_motorState.ForeColor = Color.Red;

            dataframe = "";
            dataframe += Convert.ToString(startstopState);
            dataframe += changeFormat(textBox_SetPoint.Text);
            dataframe += changeFormat(textBox_Kp.Text);
            dataframe += changeFormat(textBox_Ki.Text);
            dataframe += changeFormat(textBox_Kd.Text);
            setpoint = Convert.ToDouble(textBox_SetPoint.Text);
            dataframe += Convert.ToString(mode);

            serialPort1.Write(dataframe);
        }

        private void button_Start_Click(object sender, EventArgs e)
        {
            startstopState = 1;
            label_motorState.Text = "Motor is running";
            label_motorState.ForeColor = Color.Green;

            dataframe = "";
            dataframe += Convert.ToString(startstopState);
            dataframe += changeFormat(textBox_SetPoint.Text);
            dataframe += changeFormat(textBox_Kp.Text);
            dataframe += changeFormat(textBox_Ki.Text);
            dataframe += changeFormat(textBox_Kd.Text);
            setpoint = Convert.ToDouble(textBox_SetPoint.Text);
            dataframe += Convert.ToString(mode);

            serialPort1.Write(dataframe);
        }

        private void txtReceive_TextChanged(object sender, EventArgs e)
        {

        }

        private void ComboBox_Mode_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (ComboBox_Mode.SelectedItem.ToString() == "speed")
            {
                mode = 0;
            }
            else if (ComboBox_Mode.SelectedItem.ToString() == "position")
            {
                mode = 1;
            }
        }

        private void label8_Click(object sender, EventArgs e)
        {

        }
    }


}
