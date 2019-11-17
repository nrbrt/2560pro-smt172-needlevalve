var Avrgirl = require('@dawee/avrgirl-arduino');

var avrgirl = new Avrgirl({
  board: 'nano'
});

avrgirl.flash('mega2560-dual-smt172-needlevalve.ino.mega.hex', function (error) {
  if (error) {
    console.error(error);
  } else {
    console.info('done.');
  }
});
