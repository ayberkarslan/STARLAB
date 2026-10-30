import unittest
from engine.models import discover_models
from engine.scenarios import discover_scenarios
from engine.native import NativeKernel

class FlightTests(unittest.TestCase):
    def test_axial_rk4_launches_and_lands(self):
        models=discover_models();prepare=discover_scenarios()['flight']
        request={'assembly':[{'id':'airframe','model':'mass-component','position':0,'inputs':{'suppliedMass':4,'length':1,'radius':.05}}],
          'inputs':{'curve':[0,0,.1,200,1,200,1.1,0],'propellant':.5,'cd':.5,'area':.01,'atmosphereModel':0,'launchAltitude':0,'surfaceTemperature':288.15,'surfacePressure':101325,'windSpeed':0,'windDirection':0,'heading':0,'railAngle':0,'dt':.01,'maxTime':30}}
        _,graph,values=prepare(request,models,None);kernel=NativeKernel(NativeKernel.read_build())
        versions={node.id:'test' for node in graph.nodes};result=graph.evaluate(values,kernel,versions)
        series=result['results']['flight_series']['state']['value'];rows=[series[i:i+4] for i in range(0,len(series),4)]
        self.assertGreater(max(row[1] for row in rows),100)
        self.assertGreater(max(row[2] for row in rows),40)
        self.assertEqual(rows[-1][1],0)
        self.assertEqual(rows[-1][2],0)

    def test_pad_support_holds_underpowered_vehicle(self):
        build=NativeKernel.read_build();kernel=NativeKernel(build);output=[0.0]*200
        rows=kernel.flight_axial_solve([0,0,1,1,2,0],3,10,0,.5,.01,0,0,288.15,101325,0,0,0,0,.01,2,output,len(output))
        values=output[:int(rows)*4]
        self.assertTrue(values)
        self.assertEqual(max(values[1::4]),0)
        self.assertEqual(max(values[2::4]),0)

if __name__=='__main__':unittest.main()
