'use strict';
// Private discovery seam only. Delivered component bytes and MIDI policies stay unchanged.
const privateWorkspaceCard=()=>`<section class="product-section"><span class="section-label">YOUR LOCAL MUSIC WORKSPACE</span><h2>Take the idea further.</h2><p>Write a phrase in the corrected offline editor, inspect your sound files, or open the performance companion’s simulated demo. Keep a downloaded draft before leaving. These tools do not share a keyboard connection or save a project on the instrument.</p><div class="actions">${productExternal('/workspace/#editor','Write a phrase')}${productExternal('/workspace/#presets','Inspect a sound',true)}${productExternal('/workspace/#performance','Open performance companion',true)}</div><p class="tiny-note">Private local integration · no public tool deployment or physical MIDI qualification is implied. The editor is silent; the companion demo is simulated.</p></section>`;
const privateBaseHome=productHome,privateBaseFiles=productFiles,privateBaseCompanion=productCompanion;
productHome=()=>privateBaseHome()+privateWorkspaceCard();
productFiles=(page,kind)=>privateBaseFiles(page,kind)+privateWorkspaceCard();
productCompanion=()=>privateBaseCompanion()+privateWorkspaceCard();
