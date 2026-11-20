# Make a motion from a sentence (Kimodo, optional)

Type "wave with the right hand", wait a minute, and open the result in PinglyMotion. This uses NVIDIA's free Kimodo model on **your own NVIDIA graphics card**. It is an optional helper outside PinglyMotion; PinglyMotion works the same without it.

State (20 November 2026): new and not yet tried on a real Windows PC with an RTX card. The details for developers, the exact versions and the licences are in [tools/kimodo/README.md](../tools/kimodo/README.md).

## Can my computer do it?

- Windows 10 or 11, 64-bit.
- An NVIDIA graphics card with at least 6 GB, for example a GeForce RTX 2060 Super, 2070, 2080, 3060 or newer.
- At least 20 GB of memory (RAM), better 32 GB. The part that reads your sentence runs on the processor and needs about 16 GB by itself.
- About 40 GB of free disk space.
- An internet connection for the first setup (downloads of about 20 GB).

The helper checks all of this first and says in plain words if something is missing.

## One-time setup (about an hour, mostly waiting)

1. Get the `tools\kimodo` folder: on the PinglyMotion GitHub page press **Code > Download ZIP**, unpack it, and open `tools\kimodo`. (It is not in the PinglyMotion program zip.)
2. Double-click **`PinglyMotion-Kimodo.bat`**. If Windows warns about a file from the internet, choose **More info > Run anyway**.
3. The helper checks your graphics card, memory and disk.
4. **Meta's licence.** Kimodo reads your sentence with Meta's Llama 3 model, and Meta wants every user to accept its licence once, on the Hugging Face website. The helper opens the page. Log in or make a free account, read the licence, and accept it if you agree. Access usually comes within minutes, by e-mail.
5. **Your token.** The helper opens a second page where you make an access token (any name, type "Read", **Create token**). Copy it, go back to the helper window and paste it (right-click or Ctrl+V); it is not shown while you paste. The token stays on your computer only.
6. The helper installs PyTorch and Kimodo into its own folder. Nothing is installed for the whole computer.
7. The first motion you make also downloads the models (about 17 GB). After that, a motion takes about a minute.

## Make a motion

1. Double-click `PinglyMotion-Kimodo.bat`.
2. At **Motion:** type what the avatar does, in English, for example `wave with the right hand`. One or two actions work best; everyday movement, gestures, walking, dancing and moods ("tired", "happy", "drunk") work well.
3. At **Length in seconds** type a number from 1 to 10, or just press Enter for 4.
4. When it says **Done**, the file is in `Documents\PinglyMotion`, named after your words (`kimodo_wave_with_right_hand.bvh`). Make another one, or press Enter on an empty line to stop.

## Open it in PinglyMotion

1. **File > Import BVH from another skeleton**, then pick the file in `Documents\PinglyMotion`.
2. The window shows which joint goes where; PinglyMotion knows Kimodo's skeleton, so all body joints are matched. Press **Open the animation**.
3. Kimodo makes no finger movement: the fingers keep their pose.

## Good to know

- **Selling animations:** the Kimodo-SOMA model's licence says it can be used commercially and that NVIDIA does not own what you make with it. Read the licences yourself before you sell; this guide is not legal advice.
- **Remove it:** delete the folder `%LOCALAPPDATA%\PinglyMotion\kimodo` (paste that into the Explorer address bar). Revoke the token on huggingface.co under Settings > Access Tokens if you like.
- **Using your own key poses as a guide** for Kimodo is planned for later.
