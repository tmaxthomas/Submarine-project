using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Submarine_Ground_Station_UI
{
    public partial class Form1 : Form
    {
        //Graphics constants
        const int PISTON_OFFSET = 421;
        const int PISTON_HEIGHT = 214;
        const double BAL_WIDTH = 124;
        const int SPOOL_OFFSET = 621;
        const int SPOOL_HEIGHT = 219;
        const int SPOOL_WIDTH = 40;
        //Graphics variables
        double piston_perc = 0;
        bool fore_flooded = false;
        bool aft_flooded = false;
        bool fore_powered = true;
        bool aft_powered = true;
        double spool_perc = 1;
        
        
        
        Bitmap base_map, tank_piston, water, wire;
        Graphics g;
        public Form1()
        {
            InitializeComponent();
            String file_path = System.IO.Path.GetDirectoryName(Application.ExecutablePath);
            base_map = new Bitmap(file_path + @"\primary.bmp");
            tank_piston = new Bitmap(file_path + @"\Ballast tank piston.bmp");
            water = new Bitmap(file_path + @"\water.bmp");
            wire = new Bitmap(file_path + @"\wire.bmp");
            g = this.CreateGraphics();
        }

        private void button1_Click(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {

        }

        private void trackBar1_Scroll(object sender, EventArgs e)
        {
            piston_perc = (double)trackBar1.Value / 10;
            RenderGraphics();
        }

        private void label4_Click(object sender, EventArgs e)
        {

        }

        private void button3_Click(object sender, EventArgs e)
        {
            trackBar1.Value = 0;
            piston_perc = 0;
            fore_flooded = false;
            aft_flooded = false;
            fore_powered = true;
            aft_powered = true;
            spool_perc = 1;
            trackBar2.Value = 10;
            RenderGraphics();
        }

        private void trackBar2_Scroll_1(object sender, EventArgs e)
        {
            spool_perc = (double)trackBar2.Value / 10;
            RenderGraphics();
        }

        private void RenderGraphics()
        {
            g.DrawImage(base_map, 0, 100);
            int piston_pos = (int)(PISTON_OFFSET + BAL_WIDTH * piston_perc);
            g.DrawImage(tank_piston, piston_pos, PISTON_HEIGHT);
            //Now render the water in the tank
            for(int a = PISTON_OFFSET; a < piston_pos; a++)
            {
                g.DrawImage(water, a, PISTON_HEIGHT);
            }
            //And render the cable on the spool
            int spool_pos = (int)(SPOOL_OFFSET + SPOOL_WIDTH * spool_perc);
            for(int a = SPOOL_OFFSET; a < spool_pos; a++)
            {
                g.DrawImage(wire, a, SPOOL_HEIGHT);
            }
        }
    }

    
}
