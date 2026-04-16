include(mac/general.mac)
define(TITLE, LORAWIFI Drone Detector)dnl
include(mac/header.mac)
include(mac/styles.mac)
define(SUBTITLE, Product by Peter Glen)dnl
define(SUBBANNER, Detection Settings)
include(mac/bannerdd.mac)
include(mac/live.mac)

<script>
    function pressed_param() {
        console.log("Pressed param submit");
    for (aa = 1; aa <= 12; aa++)
        {
        //console.log("name-" + aa, "val-" + aa);
        const nameInput = document.getElementById("name-" + aa);
        const valInput = document.getElementById("val-" + aa);
        console.log("input: " + nameInput.value, " value: " + valInput.value)
        }
    const formData = new FormData(myform);
    console.log(formData);
    }
</script>

<br>
&nbsp; &nbsp; &nbsp; The detection parameters can be specified in the following
sections. The detection parameters consist of the detection name and detecton
frequenct compnents. The name is transmitted in the alarm string after
detection of a prticular item. The detection parameters consist of the 5
dominant frequencies. The order of the parameters matters, as the first
parameter matching the incoming signal will trigger detection.
<p>

<table align=center border=0 width=90%>

forloop(`ii', 1, 12,
        `<tr><td> Detected name: <td>
                    <input name=name-ii id=name-ii type=text value=name-ii>
        <td> Parameters:    <td>
                    <input name=val-ii id=val-ii size=40 type=text value=val-ii>
        ')

</table>
<p>
&nbsp; &nbsp; &nbsp; The overall detection threshold specifies the strictness
of the detection compare operation. Too strict, detection is rare, too loose
detection is frequent. This parameter may be tuned for optimum false positives
and optimum false negatives.
<p>
&nbsp; &nbsp; &nbsp; Overall detection threshold:
                    <input name=thresh id=thresh type=text value=threshthresh>

<p> <center>
    <input type=button value="Submit Parameters" onclick=pressed_param()><br>
</center>

dnl include(mac/fcc.mac)
include(mac/lowmenudd.mac)
include(mac/comm.mac)
include(mac/foot.mac)