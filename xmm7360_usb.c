/**
 * Copyright (C) 2019  Juho Vähä-Herttua
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/version.h>
 
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("Juho Vähä-Herttua <juhovh@iki.fi>");
MODULE_DESCRIPTION("A simple driver that sets the XMM7360 card from PCIe to USB mode.");
MODULE_ALIAS("pci:v00008086d00007360sv*sd*bc*sc*i*");
 
#define XMM7360 0x7360

static struct pci_device_id pci_ids[] = {
    { PCI_VDEVICE(INTEL, XMM7360), },
    { 0, }
};

static acpi_status acpi_device_reset(struct acpi_device *dev) {
    acpi_handle handle;

    handle = acpi_device_handle(dev);
    return acpi_evaluate_object(handle, "_RST", NULL, NULL);
}

static bool pcie_link_set_enabled(struct pci_dev *dev, bool enable) {
    struct pci_dev *rdev;
    u16 lnk_ctrl;
    bool changed;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    rdev = pci_find_pcie_root_port(dev);
#else
    rdev = pcie_find_root_port(dev);
#endif
    if (!rdev) {
    	pr_err(KERN_ERR "Could not find the PCIe root port for %04x:%04x\n",
               dev->vendor, dev->device);
        return false;
    }

    pcie_capability_read_word(rdev, PCI_EXP_LNKCTL, &lnk_ctrl);
    if (enable) {
        changed = lnk_ctrl & PCI_EXP_LNKCTL_LD;
        lnk_ctrl &= ~PCI_EXP_LNKCTL_LD;
    } else {
        changed = !(lnk_ctrl & PCI_EXP_LNKCTL_LD);
        lnk_ctrl |= PCI_EXP_LNKCTL_LD;
    }
    pcie_capability_write_word(rdev, PCI_EXP_LNKCTL, lnk_ctrl);

    pci_dev_put(rdev);
    return changed;
}

static bool mmx7360_set_usb_mode(struct pci_dev *dev, bool enable_usb) {
    struct acpi_device *adev;
    bool needs_reset;
    acpi_status status;

    pr_info("Found a PCI device %04x:%04x\n", dev->vendor, dev->device);
    needs_reset = pcie_link_set_enabled(dev, !enable_usb);
    if (needs_reset) {
        pr_info("Link status changed, trying to reset the device\n");

    	adev = ACPI_COMPANION(&dev->dev);
        if (!adev) {
            pr_err("Could not find the ACPI companion device\n");
            return false;
        }

        status = acpi_device_reset(adev);
        if (ACPI_FAILURE(status)) {
            pr_err("Failed to reset the ACPI companion device\n");
            return false;
        }

        pr_info("Device reset successfully\n");
    }

    return true;
}

static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id) {
    if (!mmx7360_set_usb_mode(dev, true)) {
        return -1;
    }

    return 0;
}

static void pci_remove(struct pci_dev *dev) {
    mmx7360_set_usb_mode(dev, false);
}

static struct pci_driver pci_driver = {
    .name       = "xmm7360_usb",
    .id_table   = pci_ids,
    .probe      = pci_probe,
    .remove     = pci_remove,
};

static int __init xmm7360_usb_init(void){
    int ret;

    ret = pci_register_driver(&pci_driver);
    if (ret < 0) {
        pr_err("Failed to register the PCI driver\n");
        return ret;
    }

    return 0;
}

static void __exit xmm7360_usb_exit(void){
    pci_unregister_driver(&pci_driver);
}
 
module_init(xmm7360_usb_init);
module_exit(xmm7360_usb_exit);
