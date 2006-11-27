/*****************************************************************************
 * alsa_raoppcm.c: alsa pcm driver to bridge to raop_play
 * Copyright (C) 2005 Shiro Ninomiya <shiron@snino.com>
 * 
 * Reference article:
 *   http://alsa-project.org/~iwai/writing-an-alsa-driver/index.html
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/
#include <linux/config.h>
#include <linux/poll.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>

static DECLARE_WAIT_QUEUE_HEAD(pcmout_read_wait);

typedef struct raoppcm_t{
	snd_card_t *card;
	snd_pcm_substream_t *substream;
	snd_pcm_t *pcm;
	int running;
	int readp;
}raoppcm_t;
static raoppcm_t *raoppcm_data;

#define chip_t raoppcm_t

/* hardware definition */
static snd_pcm_hardware_t snd_raoppcm_playback_hw = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED),
	.formats =          SNDRV_PCM_FMTBIT_S16_LE,
	.rates =            SNDRV_PCM_RATE_44100,
	.rate_min =         44100,
	.rate_max =         44100,
	.channels_min =     2,
	.channels_max =     2,
	.buffer_bytes_max = 32*1024,
	.period_bytes_min = 4*1024,
	.period_bytes_max = 32*1024,
	.periods_min =      1,
	.periods_max =      1024,
};

/* open callback */
static int snd_raoppcm_playback_open(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	raoppcm_t *chip = snd_pcm_substream_chip(substream);

	printk(KERN_DEBUG "%s\n",__func__);
	runtime->hw = snd_raoppcm_playback_hw;
	chip->substream=substream;
	return 0;
}

/* close callback */
static int snd_raoppcm_playback_close(snd_pcm_substream_t *substream)
{
	raoppcm_t *chip = snd_pcm_substream_chip(substream);
	chip->substream=NULL;
	printk(KERN_DEBUG "%s\n",__func__);
	return 0;
}

/* hw_params callback */
static int snd_raoppcm_pcm_hw_params(snd_pcm_substream_t *substream,
				     snd_pcm_hw_params_t *hw_params)
{
	raoppcm_t *chip = snd_pcm_substream_chip(substream);
	printk(KERN_DEBUG "%s\n",__func__);
	chip->readp=0;
	return snd_pcm_lib_malloc_pages(substream,
				      params_buffer_bytes(hw_params));
}

/* hw_free callback */
static int snd_raoppcm_pcm_hw_free(snd_pcm_substream_t *substream)
{
	printk(KERN_DEBUG "%s\n",__func__);
	return snd_pcm_lib_free_pages(substream);
}

/* prepare callback */
static int snd_raoppcm_pcm_prepare(snd_pcm_substream_t *substream)
{
	printk(KERN_DEBUG "%s\n",__func__);
	return 0;
}

/* trigger callback */
static int snd_raoppcm_pcm_trigger(snd_pcm_substream_t *substream,
				  int cmd)
{
	raoppcm_t *chip = snd_pcm_substream_chip(substream);
	printk(KERN_DEBUG "%s cmd=%d\n",__func__,cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		chip->running=1;
		wake_up(&pcmout_read_wait);
		// do something to start the PCM engine
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		chip->running=0;
		// do something to stop the PCM engine
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	default:
		return -EINVAL;
	}
	return 0;
}

/* pointer callback */
static snd_pcm_uframes_t snd_raoppcm_pcm_pointer(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	raoppcm_t *chip = snd_pcm_substream_chip(substream);
	//printk(KERN_DEBUG "%s readp=0x%x, 0x%x\n",__func__,chip->readp, (int)bytes_to_frames(runtime, chip->readp));
	wake_up(&pcmout_read_wait);
	return bytes_to_frames(runtime, chip->readp);
}

/* operators */
static snd_pcm_ops_t snd_raoppcm_playback_ops = {
	.open =        snd_raoppcm_playback_open,
	.close =       snd_raoppcm_playback_close,
	.ioctl =       snd_pcm_lib_ioctl,
	.hw_params =   snd_raoppcm_pcm_hw_params,
	.hw_free =     snd_raoppcm_pcm_hw_free,
	.prepare =     snd_raoppcm_pcm_prepare,
	.trigger =     snd_raoppcm_pcm_trigger,
	.pointer =     snd_raoppcm_pcm_pointer,
};

/* create a pcm device */
static int __devinit snd_raoppcm_new_pcm(raoppcm_t *chip)
{
	snd_pcm_t *pcm;
	int err;

	if ((err = snd_pcm_new(chip->card, "raop pcm", 0, 1, 0, &pcm)) < 0) 
		return err;
	pcm->private_data = chip;
	strcpy(pcm->name, "raop pcm");
	chip->pcm = pcm;
	/* set operators */
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_raoppcm_playback_ops);
	/* pre-allocation of buffers */
	/* NOTE: this may fail */
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
					      snd_dma_continuous_data(GFP_KERNEL),
					      64*1024, 64*1024);
	return 0;
}

/*
 * create a simple char output device to send pcm data to raop_play
 */
#define PCMOUT_NAME "pcmout"
static int pcmout_major;

static ssize_t pcmout_read(struct file *file, char *buffer, size_t count, loff_t * ppos)
{
	snd_pcm_runtime_t *runtime;
	int period_bytes, size_tob;

	if(!raoppcm_data) {
		printk(KERN_ERR "raoppcm_data==NULL\n");
		return -1;
	}
	if(!raoppcm_data->substream) {
		printk(KERN_ERR "raoppcm_data->substream==NULL\n");
		return -1;
	}
	if(wait_event_interruptible(pcmout_read_wait,raoppcm_data->running)) {
		printk(KERN_ERR "stopped by a signal\n");
		return -1;
	}

	runtime = raoppcm_data->substream->runtime;
	period_bytes = frames_to_bytes(runtime, runtime->period_size);
	if(period_bytes<=0) return -1;
/* 	printk(KERN_DEBUG "period_bytes=%d, count=%d, readp=0x%x, hw_ptr=0x%x, ap_ptr=0x%x\n", */
/* 	       period_bytes, (int)count, raoppcm_data->readp, */
/* 	       (int)runtime->status->hw_ptr, (int)runtime->control->appl_ptr); */
	count=count<period_bytes?count:period_bytes;

	if(wait_event_interruptible(pcmout_read_wait, runtime->status->hw_ptr+
				    bytes_to_frames(runtime, count) < runtime->control->appl_ptr)){
		printk(KERN_ERR "stopped by a signal\n");
		return -1;
	}

	size_tob=runtime->dma_bytes-raoppcm_data->readp;
	if(size_tob>=count){
		copy_to_user(buffer, runtime->dma_area+raoppcm_data->readp, count);
		raoppcm_data->readp+=count;
		if(raoppcm_data->readp==runtime->dma_bytes) raoppcm_data->readp=0;
		snd_pcm_period_elapsed(raoppcm_data->substream);
		return count;
	}
	copy_to_user(buffer, runtime->dma_area+raoppcm_data->readp, size_tob);
	copy_to_user(buffer+size_tob, runtime->dma_area, count-size_tob);
	raoppcm_data->readp=count-size_tob;
	snd_pcm_period_elapsed(raoppcm_data->substream);
	return count;
}

static unsigned int pcmout_poll(struct file *file, poll_table * wait)
{
	unsigned int mask=0;
	snd_pcm_runtime_t *runtime=NULL;
	if(raoppcm_data->substream) runtime=raoppcm_data->substream->runtime;
	poll_wait(file, &pcmout_read_wait, wait);
	if(!raoppcm_data->running) return 0;
	if(runtime &&
	   runtime->status->hw_ptr+runtime->period_size < runtime->control->appl_ptr)
		mask=POLLIN | POLLRDNORM;
	return mask;
}
static int pcmout_open(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "%s\n",__func__);
	return 0;
}
static int pcmout_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "%s\n",__func__);
	return 0;
}

struct file_operations pcmout_fops = {
	open: pcmout_open,
	release: pcmout_release,
	read: pcmout_read,
	poll: pcmout_poll,
};
static int pcmout_init(void)
{
	if((pcmout_major = register_chrdev(0, PCMOUT_NAME, &pcmout_fops))<0){
		printk(KERN_WARNING PCMOUT_NAME ": can't get major number\n");
		return pcmout_major;
	}
	return 0;
}
static int pcmout_creaup(void)
{
	unregister_chrdev(pcmout_major, PCMOUT_NAME);
	return 0;
}


static int __init alsa_raoppcm_init(void)
{
	int err;
	snd_card_t *card;

	card = snd_card_new(-1, "raoppcm", THIS_MODULE, sizeof(raoppcm_t));
	if (card == NULL)
		return -ENOMEM;
	raoppcm_data=(raoppcm_t*)((size_t)card+sizeof(snd_card_t));
	raoppcm_data->card = card;
	card->private_data = NULL;
	card->private_free = NULL;

	if((err=snd_raoppcm_new_pcm(raoppcm_data))) return err;

	strcpy(card->driver, "raoppcm");
	sprintf(card->shortname, "ALSA RAOPPCM");
	strcpy(card->longname, "alsa pcm driver to bridge to raop_play");

	if ((err = snd_card_register(card))) goto erexit;
	if ((err = pcmout_init())) goto erexit;
	printk( KERN_DEBUG "ALSA RAOPPCM dirver is initialized\n");
	return 0;

 erexit:
	snd_card_free(card);
	return err;
}

static void __exit alsa_raoppcm_exit(void)
{
	pcmout_creaup();
	snd_card_free(raoppcm_data->card);
}

module_init(alsa_raoppcm_init);
module_exit(alsa_raoppcm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PCM audio data bridge to raop_play");
MODULE_AUTHOR("Shiro Ninomiya");
