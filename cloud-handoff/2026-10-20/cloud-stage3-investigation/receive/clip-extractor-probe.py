from pathlib import Path
import subprocess
import tempfile
root=Path('/workspace/x0x-stage3-receive')
with tempfile.TemporaryDirectory() as tmp:
    out=Path(tmp)
    extracts=[
      ('clip_turn', r'/^static const char CK_FLD_N\[\] = /{print} /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_turn\(int n, int delta\)$/{on=1} on{print} on && /^}/{on=0}', 'firmware/src/app/main_fm1.c'),
      ('smc_input', r'/^#define SMC_Q /{on=1} /^static int smc_same\(/{on=0} on{print}', 'firmware/src/app/smc_mixer.c'),
      ('smc_turn', r'/^static void smc_knob\(/ || /^static uint8_t smc_message\(/ || /^void smc_poll\(void\)/{on=1} on{print} on && /^}/{on=0}', 'firmware/src/app/smc_mixer.c')]
    for name, program, source in extracts:
        text=subprocess.check_output(['awk',program,str(root/source)],text=True)
        (out/(name+'.inc')).write_text(text)
    subprocess.run(['cc','-g','-O1','-fsanitize=address,undefined','-std=c99','-Wall','-Wextra','-Werror','-I'+str(root/'firmware/src'),'-I'+str(out),'-o',str(out/'test'),str(root/'tests/host/smc_clip_visibility_test.c')],check=True)
    subprocess.run([str(out/'test')],check=True)
print('PASS diagnostic-only temporary extractor adjustment; candidate unchanged')
