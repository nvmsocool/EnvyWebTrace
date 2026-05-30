# EnvyWebTrace

EnvyWebTrace is a web port of the wildly popular [EnvyTrace](https://github.com/nvmsocool/EnvyTrace) software.

See the [Live Tool](https://nvmsocool.github.io/EnvyWebTrace/envywebtrace.html) here.

Screenshot:

<img src="screenshot.png"></img>

Big thanks to jnmaloney's great repo [WebGUI](https://github.com/jnmaloney/WebGui) for the template on how to get an Emscripted app up and running!

This project implements a programmable kaleidoscoic scape-time fractal, based on the work here: https://www.fractalforums.com/sierpinski-gasket/kaleidoscopic-(escape-time-ifs)/ and here: http://blog.hvidtfeldts.net/index.php/2011/06/distance-estimated-3d-fractals-part-i/

You can recreate the distance estimator algorithms using the action list for the fractal and the variables available: Scale, ItSub, Bailout, NumSubdivisions. Under the hood these are used like so:

```
Distance_Estimator(vec){
   r=vec.length
   for(i=0;i<NumSubdivisions && r<Bailout; i++)
   {
      // folds, rotations, translations, etc defined by user with actions are conducted here

      r=vec.length;
   }
   return (sqrt(r) - ItSub) * Scale^(-i);
}
```

A few of the more popular fractals have been recreated as an example in the `scene->presets` options